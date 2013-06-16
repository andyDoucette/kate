/* This file is part of the KDE and the Kate project
 *
 *   Copyright (C) 2012 Dominik Haumann <dhaumann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "katemessagewidget.h"
#include "katemessagewidget.moc"
#include "katefadeeffect.h"

#include <ktexteditor/messageinterface.h>
#include <kmessagewidget.h>

#include <kdeversion.h>
#include <kdebug.h>

#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtGui/QVBoxLayout>
#include <QToolTip>
#include <QShowEvent>

static const int s_defaultAutoHideTime = 6 * 1000;

KateMessageWidget::KateMessageWidget(QWidget* parent, bool applyFadeEffect)
  : QWidget(parent)
  , m_fadeEffect(0)
  , m_showMessageRunning(false)
  , m_hideAnimationRunning(false)
  , m_autoHideTimer(new QTimer(this))
  , m_autoHideTime(-1)
{
  QVBoxLayout* l = new QVBoxLayout();
  l->setMargin(0);

  m_messageWidget = new KMessageWidget(this);
  m_messageWidget->setCloseButtonVisible(false);

  l->addWidget(m_messageWidget);
  setLayout(l);

  // tell the widget to always use the minimum size.
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

  // install event filter so we catch the end of the hide animation
  m_messageWidget->installEventFilter(this);

  // by default, hide widgets
  m_messageWidget->hide();
  hide();

  if (applyFadeEffect) {
    m_fadeEffect = new KateFadeEffect(m_messageWidget);
  }

  // setup autoHide timer details
  m_autoHideTimer->setSingleShot(true);

#if KDE_IS_VERSION(4,10,60) // KMessageWidget::linkHovered() is new in KDE 4.11
  connect(m_messageWidget, SIGNAL(linkHovered(const QString&)), SLOT(linkHovered(const QString&)));
#endif
}

bool KateMessageWidget::eventFilter(QObject *obj, QEvent *event)
{
  if (obj == m_messageWidget && event->type() == QEvent::Hide && !event->spontaneous()) {

    // hide animation is finished
    m_hideAnimationRunning = false;
    m_autoHideTimer->stop();
    m_autoHideTime = -1;

    // if there are other messages in the queue, show next one, else hide us
    if (m_messageList.count()) {
      if (isVisible())
        showMessage(m_messageList[0]);
    } else {
      hide();
    }
  }

  return QWidget::eventFilter(obj, event);
}

void KateMessageWidget::showEvent(QShowEvent *event)
{
  if (!m_showMessageRunning &&
      !event->spontaneous() &&
      !m_messageList.isEmpty())
  {
    showMessage(m_messageList[0]);
  }
}

void KateMessageWidget::showMessage(KTextEditor::Message* message)
{
  // set text etc.
  m_messageWidget->setText(message->text());

  // connect textChanged(), so it's possible to change text on the fly
  connect(message, SIGNAL(textChanged(const QString&)),
          m_messageWidget, SLOT(setText(const QString&)), Qt::UniqueConnection);

  // the enums values do not necessarily match, hence translate with switch
  switch (message->messageType()) {
    case KTextEditor::Message::Positive:
      m_messageWidget->setMessageType(KMessageWidget::Positive);
      break;
    case KTextEditor::Message::Information:
      m_messageWidget->setMessageType(KMessageWidget::Information);
      break;
    case KTextEditor::Message::Warning:
      m_messageWidget->setMessageType(KMessageWidget::Warning);
      break;
    case KTextEditor::Message::Error:
      m_messageWidget->setMessageType(KMessageWidget::Error);
      break;
    default:
      m_messageWidget->setMessageType(KMessageWidget::Information);
      break;
  }

  // remove all actions from the message widget
  foreach (QAction* a, m_messageWidget->actions())
    m_messageWidget->removeAction(a);

  // add new actions to the message widget
  foreach (QAction* a, message->actions())
    m_messageWidget->addAction(a);

  // set word wrap of the message
  setWordWrap(message);

  // setup auto-hide timer, and start if requested
  m_autoHideTime = message->autoHide();
  m_autoHideTimer->stop();
  if (m_autoHideTime >= 0) {
    connect(m_autoHideTimer, SIGNAL(timeout()), message, SLOT(deleteLater()), Qt::UniqueConnection);
    if (message->autoHideMode() == KTextEditor::Message::Immediate) {
      m_autoHideTimer->start(m_autoHideTime == 0 ? s_defaultAutoHideTime : m_autoHideTime);
    }
  }

  // finally show us
  m_showMessageRunning = true;
  show();
  m_showMessageRunning = false;
  if (m_fadeEffect) {
    m_fadeEffect->fadeIn();
  } else {
#if KDE_VERSION >= KDE_MAKE_VERSION(4,10,0)   // work around KMessageWidget bugs
    m_messageWidget->animatedShow();
#else
    QTimer::singleShot(0, m_messageWidget, SLOT(animatedShow()));
#endif
  }
}

void KateMessageWidget::setWordWrap(KTextEditor::Message* message)
{
  // want word wrap anyway? -> ok
  if (message->wordWrap()) {
    m_messageWidget->setWordWrap(message->wordWrap());
    return;
  }

  // word wrap not wanted, that's ok if a parent widget does not exist
  if (!parentWidget()) {
    m_messageWidget->setWordWrap(false);
    return;
  }

  // word wrap not wanted -> enable word wrap if it breaks the layout otherwise
  int margin = 0;
  if (parentWidget()->layout()) {
    // get left/right margin of the layout, since we need to subtract these
    int leftMargin = 0, rightMargin = 0;
    parentWidget()->layout()->getContentsMargins(&leftMargin, 0, &rightMargin, 0);
    margin = leftMargin + rightMargin;
  }

  // if word wrap enabled, first disable it
  if (m_messageWidget->wordWrap())
    m_messageWidget->setWordWrap(false);

  // make sure the widget's size is up-to-date in its hidden state
  m_messageWidget->ensurePolished();
  m_messageWidget->adjustSize();

  // finally enable word wrap, if there is not enough free horizontal space
  const int freeSpace = (parentWidget()->width() - margin) - m_messageWidget->width();
  if (freeSpace < 0) {
//     kDebug() << "force word wrap to avoid breaking the layout" << freeSpace;
    m_messageWidget->setWordWrap(true);
  }
}

void KateMessageWidget::postMessage(KTextEditor::Message* message,
                           QList<QSharedPointer<QAction> > actions)
{
  Q_ASSERT(!m_messageHash.contains(message));
  m_messageHash[message] = actions;

  // insert message sorted after priority
  int i = 0;
  for (; i < m_messageList.count(); ++i) {
    if (message->priority() > m_messageList[i]->priority())
      break;
  }

  // queue message
  m_messageList.insert(i, message);

  if (i == 0 && !m_hideAnimationRunning) {
    // if message has higher priority than the one currently shown,
    // then hide the current one and then show the new one.
    if (m_messageWidget->isVisible()) {

      // autoHide timer may be running for currently shown message, therefore
      // simply disconnect autoHide timer to all timeout() receivers
      disconnect(m_autoHideTimer, SIGNAL(timeout()), 0, 0);
      m_autoHideTimer->stop();

      // a bit unnice: disconnetc textChanged() signal of previously visible message
      Q_ASSERT(m_messageList.size() > 1);
      disconnect(m_messageList[1], SIGNAL(textChanged(const QString&)),
                 m_messageWidget, SLOT(setText(const QString&)));

      m_hideAnimationRunning = true;
      if (m_fadeEffect) {
        m_fadeEffect->fadeOut();
      } else {
        m_messageWidget->animatedHide();
      }
    } else {
      showMessage(m_messageList[0]);
    }
  }

  // catch if the message gets deleted
  connect(message, SIGNAL(closed(KTextEditor::Message*)), SLOT(messageDestroyed(KTextEditor::Message*)));
}

void KateMessageWidget::messageDestroyed(KTextEditor::Message* message)
{
  // last moment when message is valid, since KTE::Message is already in
  // destructor we have to do the following:
  // 1. remove message from m_messageList, so we don't care about it anymore
  // 2. activate hide animation or show a new message()

  // remove widget from m_messageList
  int i = 0;
  for (; i < m_messageList.count(); ++i) {
    if (m_messageList[i] == message) {
      break;
    }
  }

  // the message must be in the list
  Q_ASSERT(i < m_messageList.count());

  // remove message
  m_messageList.removeAt(i);

  // remove message from hash -> release QActions
  Q_ASSERT(m_messageHash.contains(message));
  m_messageHash.remove(message);

  // start hide animation, or show next message
  if (m_messageWidget->isVisible()) {
    m_hideAnimationRunning = true;
    if (m_fadeEffect) {
      m_fadeEffect->fadeOut();
    } else {
      m_messageWidget->animatedHide();
    }
  } else if (i == 0 && m_messageList.count()) {
    showMessage(m_messageList[0]);
  }
}

void KateMessageWidget::startAutoHideTimer()
{
  // message does not want autohide, or timer already running
  if (!isVisible()                 // not visible, no message shown
    || m_autoHideTime < 0          // message does not want auto-hide
    || m_autoHideTimer->isActive() // auto-hide timer is already active
    || m_hideAnimationRunning      // widget is in hide animation phase
  ) {
    return;
  }

  // safety checks: the message must still still be valid
  Q_ASSERT(m_messageList.size());
  KTextEditor::Message* message = m_messageList[0];
  Q_ASSERT(message->autoHide() == m_autoHideTime);

  // start autoHide timer as requrested
  m_autoHideTimer->start(m_autoHideTime == 0 ? s_defaultAutoHideTime : m_autoHideTime);
}

void KateMessageWidget::linkHovered(const QString& link)
{
  QToolTip::showText(QCursor::pos(), link, m_messageWidget);
}

QString KateMessageWidget::text() const
{
  return m_messageWidget->text();
}
// kate: space-indent on; indent-width 2; replace-tabs on;
