#include <QApplication>
#include <QSurfaceFormat>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include "app/dashboard.h"

int main(int argc, char* argv[]) {
    // Set default OpenGL surface format before creating QApplication
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(4);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setSwapInterval(1);  // VSync
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);
    app.setApplicationName("Real-Time System Dashboard");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Dashboard");

    // Apply global dark palette
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(30, 30, 46));
    darkPalette.setColor(QPalette::WindowText, QColor(205, 214, 244));
    darkPalette.setColor(QPalette::Base, QColor(30, 30, 46));
    darkPalette.setColor(QPalette::AlternateBase, QColor(49, 50, 68));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(49, 50, 68));
    darkPalette.setColor(QPalette::ToolTipText, QColor(205, 214, 244));
    darkPalette.setColor(QPalette::Text, QColor(205, 214, 244));
    darkPalette.setColor(QPalette::Button, QColor(49, 50, 68));
    darkPalette.setColor(QPalette::ButtonText, QColor(205, 214, 244));
    darkPalette.setColor(QPalette::BrightText, QColor(243, 139, 168));
    darkPalette.setColor(QPalette::Link, QColor(137, 180, 250));
    darkPalette.setColor(QPalette::Highlight, QColor(137, 180, 250));
    darkPalette.setColor(QPalette::HighlightedText, QColor(30, 30, 46));
    app.setPalette(darkPalette);

    DashboardWindow window;

    // Attempt to load config from standard locations
    QStringList configPaths = {
        QDir::currentPath() + "/config/dashboard.json",
        QApplication::applicationDirPath() + "/config/dashboard.json",
        QDir::homePath() + "/.config/system-dashboard/dashboard.json"
    };

    for (const QString& path : configPaths) {
        if (QFileInfo::exists(path)) {
            qDebug() << "Loading config from:" << path;
            window.loadConfig(path);
            break;
        }
    }

    window.show();
    return app.exec();
}
