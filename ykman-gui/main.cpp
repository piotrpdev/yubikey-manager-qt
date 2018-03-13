#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <stdlib.h>
#include <QtGlobal>
#include <QtWidgets>

QCommandLineOption hiddenOption(const QString &name) {
    QCommandLineOption option(name);
    option.setFlags(option.flags() | QCommandLineOption::HiddenFromHelp);
    return option;
}

int main(int argc, char *argv[])
{
    // Global menubar is broken for qt5 apps in Ubuntu Unity, see:
    // https://bugs.launchpad.net/ubuntu/+source/appmenu-qt5/+bug/1323853
    // This workaround enables a local menubar.
    qputenv("UBUNTU_MENUPROXY","0");

    // Don't write .pyc files.
    qputenv("PYTHONDONTWRITEBYTECODE", "1");

    QApplication app(argc, argv);

    QString app_dir = app.applicationDirPath();
    QString main_qml = "/qml/main.qml";
    QString path_prefix;
    QString url_prefix;

    app.setApplicationName("YubiKey Manager");
    app.setApplicationVersion(APP_VERSION);
    app.setOrganizationName("Yubico");
    app.setOrganizationDomain("com.yubico");

    QCommandLineParser cliParser;
    cliParser.setApplicationDescription("Configure your YubiKey using a graphical application.");
    cliParser.addHelpOption();
    cliParser.addVersionOption();
    cliParser.addOptions({
        {"log-level", QCoreApplication::translate("main", "Enable logging at verbosity <LEVEL>: DEBUG, INFO, WARNING, ERROR, CRITICAL"), QCoreApplication::translate("main", "LEVEL")},
        {"log-file", QCoreApplication::translate("main", "Print logs to <FILE> instead of standard output; ignored without --log-level"), QCoreApplication::translate("main", "FILE")},
        hiddenOption("enable-piv-manager"),
    });

    cliParser.process(app);

    // A lock file is used, to ensure only one running instance at the time.
    QString tmpDir = QDir::tempPath();
    QLockFile lockFile(tmpDir + "/ykman-gui.lock");
    if(!lockFile.tryLock(100)) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("YubiKey Manager is already running.");
        msgBox.exec();
        return 1;
    }

    if (QFileInfo::exists(":" + main_qml)) {
        // Embedded resources
        path_prefix = ":";
        url_prefix = "qrc://";
    } else if (QFileInfo::exists(app_dir + main_qml)) {
        // Try relative to executable
        path_prefix = app_dir;
        url_prefix = app_dir;
    } else {  //Assume qml/main.qml in cwd.
        app_dir = ".";
        path_prefix = ".";
        url_prefix = ".";
    }

    app.setWindowIcon(QIcon(path_prefix + "/images/windowicon.png"));

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("appDir", app_dir);
    engine.rootContext()->setContextProperty("urlPrefix", url_prefix);
    engine.rootContext()->setContextProperty("appVersion", APP_VERSION);
    engine.rootContext()->setContextProperty("featureFlag_pivManager", cliParser.isSet("enable-piv-manager"));

    engine.load(QUrl(url_prefix + main_qml));

    if (cliParser.isSet("log-level")) {
        if (cliParser.isSet("log-file")) {
            QMetaObject::invokeMethod(engine.rootObjects().first(), "enableLoggingToFile", Q_ARG(QVariant, cliParser.value("log-level")), Q_ARG(QVariant, cliParser.value("log-file")));
        } else {
            QMetaObject::invokeMethod(engine.rootObjects().first(), "enableLogging", Q_ARG(QVariant, cliParser.value("log-level")));
        }
    } else {
        QMetaObject::invokeMethod(engine.rootObjects().first(), "disableLogging");
    }

    return app.exec();
}
