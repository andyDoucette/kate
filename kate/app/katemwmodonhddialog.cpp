/*
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.
 
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
 
    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
 
    ---
    Copyright (C) 2004, Anders Lund <anders@alweb.dk>
*/

#include "katemwmodonhddialog.h"
#include "katemwmodonhddialog.moc"

#include "katedocmanager.h"
#include "katemainwindow.h"

#include <kmessagebox.h>
#include <kprocess.h>
#include <krun.h>
#include <klocalizedstring.h>
#include <kiconloader.h>

#include <QtCore/QTemporaryFile>
#include <QtCore/QTextStream>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeWidgetItem>

class KateDocItem : public QTreeWidgetItem
{
  public:
    KateDocItem( KTextEditor::Document *doc, const QString &status, QTreeWidget *tw )
        : QTreeWidgetItem( tw ),
        document( doc )
    {
      setText( 0, doc->url().toString() );
      setText( 1, status );
      if ( ! doc->isModified() )
        setCheckState( 0, Qt::Checked );
      else
        setCheckState( 0, Qt::Unchecked );
    }
    ~KateDocItem()
  {}

    KTextEditor::Document *document;
};


KateMwModOnHdDialog::KateMwModOnHdDialog( DocVector docs, QWidget *parent, const char *name )
    : QDialog( parent ),
      m_proc( 0 ),
      m_diffFile( 0 )
{
  setWindowTitle(i18n("Documents Modified on Disk"));
  setObjectName(name);
  setModal(true);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  setLayout(mainLayout);

  // Message
  QHBoxLayout *hb = new QHBoxLayout;
  mainLayout->addLayout(hb);

  // dialog text
  QLabel *icon = new QLabel(this);
  hb->addWidget(icon);
  icon->setPixmap( DesktopIcon("dialog-warning") );

  QLabel *t = new QLabel(i18n(
                            "<qt>The documents listed below have changed on disk.<p>Select one "
                            "or more at once, and press an action button until the list is empty.</p></qt>"), this);
  hb->addWidget(t);
  hb->setStretchFactor(t, 1000);

  // Document list
  twDocuments = new QTreeWidget(this);
  mainLayout->addWidget(twDocuments);
  QStringList header;
  header << i18n("Filename") << i18n("Status on Disk");
  twDocuments->setHeaderLabels(header);
  twDocuments->setSelectionMode( QAbstractItemView::SingleSelection );
  twDocuments->setRootIsDecorated( false );

  m_stateTexts << "" << i18n("Modified") << i18n("Created") << i18n("Deleted");
  for ( int i = 0; i < docs.size(); i++ )
  {
    new KateDocItem( docs[i], m_stateTexts[ (uint)KateDocManager::self()->documentInfo( docs[i] )->modifiedOnDiscReason ], twDocuments );
  }
  twDocuments->header()->setStretchLastSection(false);
  twDocuments->header()->setSectionResizeMode(0, QHeaderView::Stretch);
  twDocuments->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

  connect(twDocuments, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(slotSelectionChanged(QTreeWidgetItem*,QTreeWidgetItem*)) );

  // Diff line
  hb = new QHBoxLayout;
  mainLayout->addLayout(hb);

  QPushButton *btnDiff = new QPushButton(QIcon::fromTheme("document-preview"), i18n("&View Difference"), this);
  btnDiff->setWhatsThis(i18n(
                          "Calculates the difference between the editor contents and the disk "
                          "file for the selected document, and shows the difference with the "
                          "default application. Requires diff(1).") );
  hb->addWidget(btnDiff);
  connect(btnDiff, SIGNAL(clicked()), this, SLOT(slotDiff()));

  // Dialog buttons
  QDialogButtonBox *buttons = new QDialogButtonBox(this);
  mainLayout->addWidget(buttons);

  QPushButton *ignoreButton = new QPushButton(QIcon::fromTheme("dialog-warning"), i18n("&Ignore Changes"));
  ignoreButton->setToolTip(i18n("Remove modified flag from selected documents"));
  buttons->addButton(ignoreButton, QDialogButtonBox::RejectRole);
  connect(ignoreButton, SIGNAL(clicked()), this, SLOT(slotIgnore()));

  QPushButton *overwriteButton = new QPushButton;
  KGuiItem::assign(overwriteButton, KStandardGuiItem::overwrite());
  overwriteButton->setToolTip(i18n("Overwrite selected documents, discarding disk changes"));
  connect(overwriteButton, SIGNAL(clicked()), this, SLOT(slotOverwrite()));


  QPushButton *reloadButton = new QPushButton(QIcon::fromTheme("view-refresh"), i18n("&Reload"));
  reloadButton->setDefault(true);
  reloadButton->setToolTip(i18n("Reload selected documents from disk"));
  connect(reloadButton, SIGNAL(clicked()), this, SLOT(slotReload()));

  slotSelectionChanged(NULL, NULL);
}

KateMwModOnHdDialog::~KateMwModOnHdDialog()
{
  KateMainWindow::unsetModifiedOnDiscDialogIfIf(this);
  delete m_proc;
  m_proc = 0;
  if (m_diffFile) {
    m_diffFile->setAutoRemove(true);
    delete m_diffFile;
    m_diffFile = 0;
  }
}

void KateMwModOnHdDialog::slotIgnore()
{
  handleSelected( Ignore );
}

void KateMwModOnHdDialog::slotOverwrite()
{
  handleSelected( Overwrite );
}

void KateMwModOnHdDialog::slotReload()
{
  handleSelected( Reload );
}

void KateMwModOnHdDialog::handleSelected( int action )
{
  // collect all items we can remove
  QList<QTreeWidgetItem *> itemsToDelete;
  for ( QTreeWidgetItemIterator it ( twDocuments ); *it; ++it )
  {
    KateDocItem *item = (KateDocItem *) * it;
    if ( item->checkState(0) == Qt::Checked )
    {
      KTextEditor::ModificationInterface::ModifiedOnDiskReason reason = KateDocManager::self()->documentInfo( item->document )->modifiedOnDiscReason;
      bool success = true;

      if (KTextEditor::ModificationInterface *iface = qobject_cast<KTextEditor::ModificationInterface *>(item->document))
        iface->setModifiedOnDisk( KTextEditor::ModificationInterface::OnDiskUnmodified );

      switch ( action )
      {
        case Overwrite:
          success = item->document->save();
          if ( ! success )
          {
            KMessageBox::sorry( this,
                                i18n("Could not save the document \n'%1'",
                                     item->document->url().toString() ) );
          }
          break;

        case Reload:
          item->document->documentReload();
          break;

        default:
          break;
      }

      if ( success )
        itemsToDelete.append( item );
      else
      {
        if (KTextEditor::ModificationInterface *iface = qobject_cast<KTextEditor::ModificationInterface *>(item->document))
          iface->setModifiedOnDisk( reason );
      }
    }
  }

  // remove the marked items
  for (int i = 0; i < itemsToDelete.count(); ++i)
    delete itemsToDelete[i];

// any documents left unhandled?
  if ( ! twDocuments->topLevelItemCount() )
    accept();
}

void KateMwModOnHdDialog::slotSelectionChanged(QTreeWidgetItem *current, QTreeWidgetItem *)
{
  // set the diff button enabled
  btnDiff->setEnabled( current &&
                       KateDocManager::self()->documentInfo( (static_cast<KateDocItem*>(current))->document )->modifiedOnDiscReason != KTextEditor::ModificationInterface::OnDiskDeleted );
}

// ### the code below is slightly modified from kdelibs/kate/part/katedialogs,
// class KateModOnHdPrompt.
void KateMwModOnHdDialog::slotDiff()
{
  if ( !btnDiff->isEnabled()) // diff button already pressed, proc not finished yet
    return;

  if ( ! twDocuments->currentItem() )
    return;

  KTextEditor::Document *doc = (static_cast<KateDocItem*>(twDocuments->currentItem()))->document;

  // don't try to diff a deleted file
  if ( KateDocManager::self()->documentInfo( doc )->modifiedOnDiscReason == KTextEditor::ModificationInterface::OnDiskDeleted )
    return;

  if (m_diffFile)
    return;

  m_diffFile = new QTemporaryFile();
  m_diffFile->open();

  // Start a KProcess that creates a diff
  m_proc = new KProcess( this );
  m_proc->setOutputChannelMode( KProcess::MergedChannels );
  *m_proc << "diff" << "-ub" << "-" << doc->url().toLocalFile();
  connect( m_proc, SIGNAL(readyRead()), this, SLOT(slotDataAvailable()) );
  connect( m_proc, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(slotPDone()) );

  setCursor( Qt::WaitCursor );
  btnDiff->setEnabled(false);

  m_proc->start();

  QTextStream ts(m_proc);
  int lastln = doc->lines();
  for ( int l = 0; l < lastln; ++l )
    ts << doc->line( l ) << '\n';
  ts.flush();
  m_proc->closeWriteChannel();
}

void KateMwModOnHdDialog::slotDataAvailable()
{
  m_diffFile->write(m_proc->readAll());
}

void KateMwModOnHdDialog::slotPDone()
{
  setCursor( Qt::ArrowCursor );
  slotSelectionChanged(twDocuments->currentItem(), 0);

  const QProcess::ExitStatus es = m_proc->exitStatus();
  delete m_proc;
  m_proc = 0;

  if ( es != QProcess::NormalExit )
  {
    KMessageBox::sorry( this,
                        i18n("The diff command failed. Please make sure that "
                             "diff(1) is installed and in your PATH."),
                        i18n("Error Creating Diff") );
    delete m_diffFile;
    m_diffFile = 0;
    return;
  }

  if ( m_diffFile->size() == 0 )
  {
    KMessageBox::information( this,
                              i18n("Besides white space changes, the files are identical."),
                              i18n("Diff Output") );
    delete m_diffFile;
    m_diffFile = 0;
    return;
  }

  m_diffFile->setAutoRemove(false);
  QUrl url = QUrl::fromLocalFile(m_diffFile->fileName());
  delete m_diffFile;
  m_diffFile = 0;

  // KRun::runUrl should delete the file, once the client exits
  KRun::runUrl( url, "text/x-patch", this, true );
}

void KateMwModOnHdDialog::addDocument(KTextEditor::Document *doc)
{
    for ( QTreeWidgetItemIterator it ( twDocuments ); *it; ++it )
    {
      KateDocItem *item = (KateDocItem *) * it;
      if (item->document==doc)
      {
        delete item;
        break;
      }
    }
    new KateDocItem( doc, m_stateTexts[ (uint)KateDocManager::self()->documentInfo( doc )->modifiedOnDiscReason ], twDocuments );
}

void KateMwModOnHdDialog::keyPressEvent( QKeyEvent *event )
{
  if ( event->modifiers() == 0 )
  {
    if ( event->key() == Qt::Key_Escape )
    {
      event->accept();
      return;
    }
  }
  QDialog::keyPressEvent(event);
}

void KateMwModOnHdDialog::closeEvent( QCloseEvent *e )
{
  if ( ! twDocuments->topLevelItemCount() )
    QDialog::closeEvent(e);
  else e->ignore();
}
// kate: space-indent on; indent-width 2; replace-tabs on;
