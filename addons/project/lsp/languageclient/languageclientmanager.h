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

#include "client.h"

#include <languageserverprotocol/diagnostics.h>
#include <languageserverprotocol/languagefeatures.h>
#include <languageserverprotocol/textsynchronization.h>

#include "kateproject.h"

class KateProjectPlugin;

namespace LanguageClient {

class LanguageClientMark;

class LanguageClientManager : public QObject
{
    Q_OBJECT
public:
    LanguageClientManager(KateProjectPlugin *parent);
    LanguageClientManager(const LanguageClientManager &other) = delete;
    LanguageClientManager(LanguageClientManager &&other) = delete;
    ~LanguageClientManager() override;

    void startClient(Client *client);
    QVector<Client *> clients();

    void addExclusiveRequest(const LanguageServerProtocol::MessageId &id, Client *client);
    void reportFinished(const LanguageServerProtocol::MessageId &id, Client *byClient);

    void deleteClient(Client *client);

    void shutdown();

#if 0
    static QList<Client *> clientsSupportingDocument(const TextEditor::TextDocument *doc);

    static void applySettings();
    static QList<BaseSettings *> currentSettings();
    static Client *clientForSetting(const BaseSettings *setting);
#endif
signals:
    void shutdownFinished();

private Q_SLOTS:
    /**
     * A new project got created.
     * @param project new created project
     */
    void projectCreated(KateProject *project);

private:

    bool initClients();

#if 0
    void editorOpened(Core::IEditor *editor);
    void documentOpened(Core::IDocument *document);
    void documentClosed(Core::IDocument *document);
    void documentContentsSaved(Core::IDocument *document);
    void documentWillSave(Core::IDocument *document);
    void findLinkAt(const Utils::FileName &filePath, const QTextCursor &cursor,
                    Utils::ProcessLinkCallback callback);
    void findUsages(const Utils::FileName &filePath, const QTextCursor &cursor);

    void projectAdded(ProjectExplorer::Project *project);
    void projectRemoved(ProjectExplorer::Project *project);
#endif

    /**
     * the project plugin we belong to
     * allows access to signals for project management
     */
    KateProjectPlugin * const m_projectPlugin = nullptr;

    /**
     * clients inited?
     */
    bool m_clientsInited = false;

    QVector<Client *> reachableClients();
    void sendToAllReachableServers(const LanguageServerProtocol::IContent &content);

    void clientFinished(Client *client);

    bool m_shuttingDown = false;
    QVector<Client *> m_clients;
    QHash<LanguageServerProtocol::MessageId, QList<Client *>> m_exclusiveRequests;
};
} // namespace LanguageClient
