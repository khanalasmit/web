#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include "form.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_InitialButton_clicked();
    void on_QuitButton_clicked();
    void on_GOTOMAINWINDOW_clicked();
    void on_pushButtonday1_clicked();

    void on_pushButtonday2_clicked();

    void on_pushButtonday3_clicked();

    void on_pushButtonday4_clicked();
private:
    Ui::MainWindow *ui;
    Form *form;
    QLabel *firstdisplay;
    QLabel *condition;
    QLabel *date;
    QLabel *humidity;
    QLabel *currentlocation;
    QLabel *currenttemperature;
    QLabel *currentcondition;
    QLabel *currentweathericon;
    QLabel *currentwind;
    QLabel *currentpressure;

    QLabel *labelTime1;
    QLabel *labeltemperature1;
    QLabel *labelcondition1;

    QLabel *labelTime2;
    QLabel *labeltemperature2;
    QLabel *labelcondition2;

    QLabel *labelcondition3;
    QLabel *labelTime3;
    QLabel *labeltemperature3;

    QLabel *labelcondition4;
    QLabel *labelTime4;
    QLabel *labeltemperature4;

    QLabel *labelcondition5;
    QLabel *labelTime5;
    QLabel *labeltemperature5;

    QLabel *labelcondition6;
    QLabel *labelTime6;
    QLabel *labeltemperature6;

    QLabel *labelcondition7;
    QLabel *labelTime7;
    QLabel *labeltemperature7;




};

#endif // MAINWINDOW_H
