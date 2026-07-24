#include <QApplication>
#include <QStyleHints>
#include <QPalette>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSplashScreen>
#include <QTimer>
#include <QPixmap>
#include <QFontDatabase>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuickStyle>

#include "AppContext.h"
#include "ShowViewModel.h"
#include "SlideDeckViewModel.h"
#include "ArrangementViewModel.h"
#include "LyricsEditorViewModel.h"
#include "LayoutEditorViewModel.h"
#include "DisplayEngine.h"

int main(int argc, char *argv[]) {
    // Suppress noisy FFmpeg, Qt Multimedia, and Wayland text input logs
    qputenv("QT_LOGGING_RULES", "qt.multimedia.*=false;qt.qpa.wayland.textinput=false");
    
    // Suppress low-level FFmpeg and hardware decoder stderr spam
    qputenv("AV_LOG_LEVEL", "quiet");
    qputenv("VDPAU_LOG", "0");
    qputenv("LIBVA_MESSAGING_LEVEL", "0");

    // Force Qt Quick Controls to use the Fusion style
    // (Bypasses CMake's qtquickcontrols2.conf resource nesting bug)
    qputenv("QT_QUICK_CONTROLS_STYLE", "Fusion");

    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icon.png"));
    app.setApplicationName("FOHMediaShow");
    app.setStyle("Fusion");
    QQuickStyle::setStyle("Fusion");
    app.styleHints()->setColorScheme(Qt::ColorScheme::Dark);

    // Build a true Dark Palette for the Fusion style (fixes white comboboxes/text inputs)
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(42, 42, 42));
    darkPalette.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, Qt::darkGray);
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);
    app.setPalette(darkPalette);

    // Load fonts
    QFontDatabase::addApplicationFont(":/fohmedia/fonts/Inter-VariableFont_slnt,wght.ttf");
    QFontDatabase::addApplicationFont(":/fohmedia/fonts/RobotoMono-VariableFont_wght.ttf");
    
    // Explicitly enforce 'Inter' at 11pt globally so Windows and Linux render identically
    // (Bypasses the OS-level default font differences like Segoe UI vs Ubuntu/Cantarell)
    QFont defaultFont("Inter");
    defaultFont.setPointSize(11);
    app.setFont(defaultFont);

    // Formal application namespace for settings
    QCoreApplication::setApplicationName("FOHMedia");

    // Check settings to see if HW Video Decoding is explicitly disabled by the user
    // We must parse the JSON manually here because AppContext/SettingsManager aren't created yet.
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QFile settingsFile(QDir(configPath).absoluteFilePath("settings.json"));
    bool disableHwVideo = false;
    if (settingsFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(settingsFile.readAll());
        if (doc.isObject() && doc.object().value("disableHwVideo").toBool()) {
            disableHwVideo = true;
        }
    }
    
    if (disableHwVideo) {
        // Disable hardware acceleration for video decoding (fixes DXVA2 initialization errors on Windows VMs)
        qputenv("QT_FFMPEG_DECODING_HW_DEVICE_TYPES", "");
    }

    // Optional splash screen (useful if loading takes a moment)
    QSplashScreen* splash = nullptr;
    QPixmap splashPix(":/fohmedia/images/splash.png");
    if (!splashPix.isNull()) {
        splash = new QSplashScreen(splashPix);
        splash->show();
        app.processEvents();
    }

    QQmlApplicationEngine qmlEngine;

    // Create the AppContext and explicitly register it as a singleton.
    // This circumvents the issue where qt_add_qml_module fails to scan QML_ELEMENT 
    // inside the fohmedia_core static library.
    AppContext* appContext = new AppContext(&qmlEngine, &app);
    qmlRegisterSingletonInstance("fohmedia", 1, 0, "AppContext", appContext);

    // Register the underlying types so QML can read their properties/signals
    qmlRegisterUncreatableType<ShowViewModel>("fohmedia", 1, 0, "ShowViewModel", "Provided by AppContext");
    qmlRegisterUncreatableType<SlideDeckViewModel>("fohmedia", 1, 0, "SlideDeckViewModel", "Provided by AppContext");
    qmlRegisterUncreatableType<ArrangementViewModel>("fohmedia", 1, 0, "ArrangementViewModel", "Provided by AppContext");
    qmlRegisterUncreatableType<LyricsEditorViewModel>("fohmedia", 1, 0, "LyricsEditorViewModel", "Provided by AppContext");
    qmlRegisterUncreatableType<LayoutEditorViewModel>("fohmedia", 1, 0, "LayoutEditorViewModel", "Provided by AppContext");
    qmlRegisterUncreatableType<DisplayEngine>("fohmedia", 1, 0, "DisplayEngine", "Provided by AppContext");

    using namespace Qt::StringLiterals;
    const QUrl url(u"qrc:/fohmedia/qml/Main.qml"_s);
    QObject::connect(&qmlEngine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    qmlEngine.load(url);

    if (splash) {
        splash->finish(nullptr);
        delete splash;
    }

    // Run Qt event loop
    return app.exec();
}
