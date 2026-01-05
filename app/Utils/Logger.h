#ifndef FLOODSIM_LOGGER_H
#define FLOODSIM_LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>
#include <QDir>

/**
 * @brief singleton logger class that logs to console and file
 *
 * logs are saved to logs/ directory with timestamp filename (its in build dir)
 */
class Logger {
public:
    static Logger& instance() {
        static Logger instance;
        return instance;
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void log(const QString& message) {
        QString timestampedMessage = QString("[%1] %2")
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
            .arg(message);

        qDebug().noquote() << timestampedMessage;

        if (logFile.isOpen() && logStream) {
            *logStream << timestampedMessage << "\n";
            logStream->flush();
        } else {
            qWarning() << "Log file is not open. Message not logged to file.";
        }
    }

    template<typename... Args>
    void log(const QString& format, Args... args) {
        log(QString::asprintf(format.toUtf8().constData(), args...));
    }

    ~Logger() {
        if (logFile.isOpen()) {
            logFile.close();
        }
        delete logStream;
    }

private:
    Logger() : logStream(nullptr) {
        QDir logsDir("logs");
        if (!logsDir.exists()) {
            logsDir.mkpath(".");
        }

        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
        QString filename = QString("logs/floodsim_%1.log").arg(timestamp);

        logFile.setFileName(filename);
        if (logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            logStream = new QTextStream(&logFile);
            log("=== FloodSim session started ===");
        } else {
            qWarning() << "Failed to open log file:" << filename;
        }
    }

    QFile logFile;
    QTextStream* logStream;
};

// convenience macro for shorter logging calls
#define LOG(msg) Logger::instance().log(msg)

#endif //FLOODSIM_LOGGER_H