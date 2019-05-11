/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <languageserverprotocol/client.h>
#include <languageserverprotocol/diagnostics.h>
#include <languageserverprotocol/initializemessages.h>
#include <languageserverprotocol/languagefeatures.h>
#include <languageserverprotocol/messages.h>
#include <languageserverprotocol/shutdownmessages.h>
#include <languageserverprotocol/textsynchronization.h>

#include <QBuffer>
#include <QHash>
#include <QProcess>
#include <QJsonDocument>
#include <QTextCursor>

namespace LanguageClient {

class BaseClientInterface;

class Client : public QObject
{
    Q_OBJECT

public:
    explicit Client(BaseClientInterface *clientInterface); // takes ownership
     ~Client() override;

    Client(const Client &) = delete;
    Client(Client &&) = delete;
    Client &operator=(const Client &) = delete;
    Client &operator=(Client &&) = delete;

    enum State {
        Uninitialized,
        InitializeRequested,
        Initialized,
        ShutdownRequested,
        Shutdown,
        Error
    };

    void initialize();
    void shutdown();
    State state() const;
    bool reachable() const { return m_state == Initialized; }
    bool start();

protected:
    void setError(const QString &message);
    void handleMessage(const LanguageServerProtocol::BaseMessage &message);

    void log(const QString &message);
    template<typename Error>
    void log(const LanguageServerProtocol::ResponseError<Error> &responseError)
    { log(responseError.toString()); }


signals:
    void initialized(LanguageServerProtocol::ServerCapabilities capabilities);
    void finished();

private:
    void intializeCallback(const LanguageServerProtocol::InitializeRequest::Response &initResponse);

    void sendContent(const LanguageServerProtocol::IContent &content);
    void sendContent(const LanguageServerProtocol::DocumentUri &uri,
                     const LanguageServerProtocol::IContent &content);
    void cancelRequest(const LanguageServerProtocol::MessageId &id);

    void handleResponse(const LanguageServerProtocol::MessageId &id, const QByteArray &content,
                        QTextCodec *codec);
    void handleMethod(const QString &method, LanguageServerProtocol::MessageId id,
                      const LanguageServerProtocol::IContent *content);
#if 0
    // document synchronization
    bool openDocument(Core::IDocument *document);
    void closeDocument(const LanguageServerProtocol::DidCloseTextDocumentParams &params);
    bool documentOpen(const Core::IDocument *document) const;
    void documentContentsSaved(Core::IDocument *document);
    void documentWillSave(Core::IDocument *document);
    void documentContentsChanged(Core::IDocument *document);
    void registerCapabilities(const QList<LanguageServerProtocol::Registration> &registrations);
    void unregisterCapabilities(const QList<LanguageServerProtocol::Unregistration> &unregistrations);
    bool findLinkAt(LanguageServerProtocol::GotoDefinitionRequest &request);
    bool findUsages(LanguageServerProtocol::FindReferencesRequest &request);
    void requestDocumentSymbols(TextEditor::TextDocument *document);
    void cursorPositionChanged(TextEditor::TextEditorWidget *widget);

    void requestCodeActions(const LanguageServerProtocol::DocumentUri &uri,
                            const QList<LanguageServerProtocol::Diagnostic> &diagnostics);
    void requestCodeActions(const LanguageServerProtocol::CodeActionRequest &request);
    void handleCodeActionResponse(const LanguageServerProtocol::CodeActionRequest::Response &response,
                                  const LanguageServerProtocol::DocumentUri &uri);
    void executeCommand(const LanguageServerProtocol::Command &command);

    // workspace control
    void projectOpened(ProjectExplorer::Project *project);
    void projectClosed(ProjectExplorer::Project *project);

    void setSupportedLanguage(const LanguageFilter &filter);
    bool isSupportedDocument(const Core::IDocument *document) const;
    bool isSupportedFile(const Utils::FileName &filePath, const QString &mimeType) const;
    bool isSupportedUri(const LanguageServerProtocol::DocumentUri &uri) const;

    void setName(const QString &name) { m_displayName = name; }
    QString name() const { return m_displayName; }

    Core::Id id() const { return m_id; }

    bool needsRestart(const BaseSettings *) const;

    QList<LanguageServerProtocol::Diagnostic> diagnosticsAt(
        const LanguageServerProtocol::DocumentUri &uri,
        const LanguageServerProtocol::Range &range) const;

    bool reset();

    const LanguageServerProtocol::ServerCapabilities &capabilities() const;
    const DynamicCapabilities &dynamicCapabilities() const;
    const BaseClientInterface *clientInterface() const;
    DocumentSymbolCache *documentSymbolCache();

private:

    void handleDiagnostics(const LanguageServerProtocol::PublishDiagnosticsParams &params);

    void shutDownCallback(const LanguageServerProtocol::ShutdownRequest::Response &shutdownResponse);
    bool sendWorkspceFolderChanges() const;
    void log(const LanguageServerProtocol::ShowMessageParams &message,
             Core::MessageManager::PrintToOutputPaneFlag flag = Core::MessageManager::NoModeSwitch);

    void showMessageBox(const LanguageServerProtocol::ShowMessageRequestParams &message,
                        const LanguageServerProtocol::MessageId &id);

    void showDiagnostics(const LanguageServerProtocol::DocumentUri &uri);
    void removeDiagnostics(const LanguageServerProtocol::DocumentUri &uri);

    LanguageFilter m_languagFilter;
    QList<Utils::FileName> m_openedDocument;
    Core::Id m_id;
    DynamicCapabilities m_dynamicCapabilities;
    LanguageClientCompletionAssistProvider m_completionProvider;
    LanguageClientQuickFixProvider m_quickFixProvider;
    QSet<TextEditor::TextDocument *> m_resetAssistProvider;
    QHash<LanguageServerProtocol::DocumentUri, LanguageServerProtocol::MessageId> m_highlightRequests;
    int m_restartsLeft = 5;
    QMap<LanguageServerProtocol::DocumentUri, QList<TextMark *>> m_diagnostics;
    DocumentSymbolCache m_documentSymbolCache;
#endif
    QString m_displayName;
    QHash<LanguageServerProtocol::MessageId, LanguageServerProtocol::ResponseHandler> m_responseHandlers;
    QScopedPointer<BaseClientInterface> m_clientInterface;
    State m_state = Uninitialized;
    LanguageServerProtocol::ServerCapabilities m_serverCapabilities;
    using ContentHandler = std::function<void(const QByteArray &, QTextCodec *, QString &,
                                              LanguageServerProtocol::ResponseHandlers,
                                              LanguageServerProtocol::MethodHandler)>;

    QHash<QByteArray, ContentHandler> m_contentHandler;
};

} // namespace LanguageClient
