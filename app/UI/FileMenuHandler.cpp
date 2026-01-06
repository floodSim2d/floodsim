#include "FileMenuHandler.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QWidget>

#include "../Simulation/Grid/Grid.h"
#include "../Renderer/OpenGLRenderer.h"

FileMenuHandler::FileMenuHandler(Grid* grid, OpenGLRenderer* renderer, QWidget* parent)
    : QObject(parent),
      grid(grid),
      renderer(renderer),
      parentWidget(parent)
{
}

void FileMenuHandler::handleNew() {
    grid->clearHeightmap();
    emit statusMessageRequested("Utworzono nową mapę");
}

void FileMenuHandler::handleOpen() {
    QString path = QFileDialog::getOpenFileName(
        parentWidget,
        "Wczytaj mapę",
        "",
        "Mapa (*.map)"
    );

    if (path.isEmpty()) {
        return;
    }

    const auto widthBefore = grid->getWidth();
    const auto heightBefore = grid->getHeight();

    if (!grid->loadHeightmap(path)) {
        emit errorMessageRequested(
            "Błąd wczytywania",
            "Nie udało się wczytać pliku mapy.\n\n"
            "Plik może być uszkodzony lub mieć nieprawidłowy format."
        );
        emit statusMessageRequested("Błąd wczytywania pliku");
        return;
    }

    // reset camera if grid size changed
    if (grid->getWidth() != widthBefore || grid->getHeight() != heightBefore) {
        renderer->resetCamera();
    }

    emit statusMessageRequested(QString("Wczytano mapę: %1").arg(path));
}

void FileMenuHandler::handleSave() {
    QString path = QFileDialog::getSaveFileName(
        parentWidget,
        "Zapisz mapę",
        "",
        "Mapa (*.map)"
    );

    if (!path.isEmpty()) {
        grid->saveHeightmap(path);
        emit statusMessageRequested(QString("Zapisano mapę: %1").arg(path));
    }
}

