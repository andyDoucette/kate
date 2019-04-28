project(kateprojectplugin)

find_package(KF5NewStuff ${KF5_DEP_VERSION} REQUIRED) # For KMoreTools

# hacks to make LSP stuff from QtCreator compile
set(CMAKE_CXX_STANDARD 14)
remove_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_FROM_BYTEARRAY -DQT_NO_CAST_TO_ASCII -DQT_NO_SIGNALS_SLOTS_KEYWORDS)
kde_enable_exceptions()

include_directories( ${CMAKE_CURRENT_BINARY_DIR} lsp )

add_definitions(-DTRANSLATION_DOMAIN=\"kateproject\")
set(kateprojectplugin_PART_SRCS
  fileutil.cpp
  kateprojectplugin.cpp
  kateprojectpluginview.cpp
  kateproject.cpp
  kateprojectworker.cpp
  kateprojectitem.cpp
  kateprojectview.cpp
  kateprojectviewtree.cpp
  kateprojecttreeviewcontextmenu.cpp
  kateprojectinfoview.cpp
  kateprojectcompletion.cpp
  kateprojectindex.cpp
  kateprojectinfoviewindex.cpp
  kateprojectinfoviewterminal.cpp
  kateprojectinfoviewcodeanalysis.cpp
  kateprojectinfoviewnotes.cpp
  kateprojectconfigpage.cpp
  kateprojectcodeanalysistool.cpp
  tools/kateprojectcodeanalysistoolcppcheck.cpp
  tools/kateprojectcodeanalysistoolflake8.cpp
  tools/kateprojectcodeanalysistoolshellcheck.cpp
  tools/kateprojectcodeanalysisselector.cpp

  # language server protocol from Qt Creator
  lsp/languageserverprotocol/basemessage.cpp
  lsp/languageserverprotocol/client.cpp
  lsp/languageserverprotocol/diagnostics.cpp
  lsp/languageserverprotocol/jsonobject.cpp
  lsp/languageserverprotocol/languagefeatures.cpp
  lsp/languageserverprotocol/lsputils.cpp
  lsp/languageserverprotocol/servercapabilities.cpp
  lsp/languageserverprotocol/textsynchronization.cpp
  lsp/languageserverprotocol/clientcapabilities.cpp
  lsp/languageserverprotocol/completion.cpp
  lsp/languageserverprotocol/initializemessages.cpp
  lsp/languageserverprotocol/jsonrpcmessages.cpp
  lsp/languageserverprotocol/lsptypes.cpp
  lsp/languageserverprotocol/messages.cpp
  lsp/languageserverprotocol/shutdownmessages.cpp
  lsp/languageserverprotocol/workspace.cpp
  lsp/utils/fileutils.cpp
  lsp/utils/hostosinfo.cpp
  lsp/utils/qtcassert.cpp
  lsp/utils/savefile.cpp
  lsp/utils/textutils.cpp
  lsp/utils/mimetypes/mimedatabase.cpp
  lsp/utils/mimetypes/mimeglobpattern.cpp
  lsp/utils/mimetypes/mimemagicrule.cpp
  lsp/utils/mimetypes/mimemagicrulematcher.cpp
  lsp/utils/mimetypes/mimeprovider.cpp
  lsp/utils/mimetypes/mimetype.cpp
  lsp/utils/mimetypes/mimetypeparser.cpp
  lsp/languageclient/languageclientinterface.cpp

)

# resource for ui file and stuff
qt5_add_resources(kateprojectplugin_PART_SRCS plugin.qrc)

add_library(kateprojectplugin MODULE ${kateprojectplugin_PART_SRCS})
kcoreaddons_desktop_to_json (kateprojectplugin kateprojectplugin.desktop)
target_link_libraries(kateprojectplugin
    KF5::TextEditor
    KF5::Parts KF5::I18n
    KF5::GuiAddons
    KF5::ItemViews KF5::ItemModels KF5::IconThemes KF5::ThreadWeaver
    KF5::NewStuff # For KMoreTools
)

########### install files ###############
install(TARGETS kateprojectplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )
install( FILES kateproject.example DESTINATION  ${DATA_INSTALL_DIR}/kateproject )

############# unit tests ################
if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()
