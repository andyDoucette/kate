project(kateprojectplugin)

find_package(KF5NewStuff ${KF5_DEP_VERSION} REQUIRED) # For KMoreTools

# hacks to make LSP stuff from QtCreator compile
set(CMAKE_CXX_STANDARD 14)
remove_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_FROM_BYTEARRAY -DQT_NO_CAST_TO_ASCII)
kde_enable_exceptions()

include_directories( ${CMAKE_CURRENT_BINARY_DIR} libs )

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
  libs/languageserverprotocol/basemessage.cpp
  libs/languageserverprotocol/client.cpp
  libs/languageserverprotocol/diagnostics.cpp
  libs/languageserverprotocol/jsonobject.cpp
  libs/languageserverprotocol/languagefeatures.cpp
  libs/languageserverprotocol/lsputils.cpp
  libs/languageserverprotocol/servercapabilities.cpp
  libs/languageserverprotocol/textsynchronization.cpp
  libs/languageserverprotocol/clientcapabilities.cpp
  libs/languageserverprotocol/completion.cpp
  libs/languageserverprotocol/initializemessages.cpp
  libs/languageserverprotocol/jsonrpcmessages.cpp
  libs/languageserverprotocol/lsptypes.cpp
  libs/languageserverprotocol/messages.cpp
  libs/languageserverprotocol/shutdownmessages.cpp
  libs/languageserverprotocol/workspace.cpp
  libs/utils/fileutils.cpp
  libs/utils/hostosinfo.cpp
  libs/utils/qtcassert.cpp
  libs/utils/savefile.cpp
  libs/utils/textutils.cpp
  libs/utils/mimetypes/mimedatabase.cpp
  libs/utils/mimetypes/mimeglobpattern.cpp
  libs/utils/mimetypes/mimemagicrule.cpp
  libs/utils/mimetypes/mimemagicrulematcher.cpp
  libs/utils/mimetypes/mimeprovider.cpp
  libs/utils/mimetypes/mimetype.cpp
  libs/utils/mimetypes/mimetypeparser.cpp

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
