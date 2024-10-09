#include "mainwindow.h"
#include <QApplication>
#include <QLabel>
#include <QTimer>
#include <QMovie>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Create QLabel for splash screen
    QLabel splashScreen;
    QMovie *movie = new QMovie("C:/Users/Acer/OneDrive/Documents/oop project/weather images/loadingi.gif");

    // Set the scaled size for the GIF
    movie->setScaledSize(QSize(800, 800));  // Scaling the GIF to 800x800 while keeping the aspect ratio
    splashScreen.setMovie(movie);
    movie->start();

    splashScreen.show();

    // Set a timer to close the splash screen after 2 seconds
    QTimer::singleShot(2000, &splashScreen, &QWidget::close);

    // Set a timer to show the main window after the splash screen closes
    MainWindow w;
    QTimer::singleShot(2000, &w, &MainWindow::show);

    return a.exec();
}
