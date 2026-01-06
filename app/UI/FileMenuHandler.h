#ifndef FLOODSIM_FILEMENUHANDLER_H
#define FLOODSIM_FILEMENUHANDLER_H

#include <QObject>

class Grid;
class OpenGLRenderer;
class QWidget;

/**
 * @brief handler for file menu operations
 *
 * responsible for:
 * - creating new heightmaps
 * - loading heightmaps from files
 * - saving heightmaps to files
 * - managing file dialogs
 */
class FileMenuHandler : public QObject {
    Q_OBJECT

public:
    explicit FileMenuHandler(Grid* grid, OpenGLRenderer* renderer, QWidget* parent = nullptr);
    ~FileMenuHandler() override = default;

public slots:
    void handleNew();
    void handleOpen();
    void handleSave();

signals:
    /**
     * @brief status message should be displayed
     */
    void statusMessageRequested(const QString& message);

    /**
     * @brief error message should be displayed
     */
    void errorMessageRequested(const QString& title, const QString& message);

private:
    Grid* grid;
    OpenGLRenderer* renderer;
    QWidget* parentWidget;
};

#endif // FLOODSIM_FILEMENUHANDLER_H

