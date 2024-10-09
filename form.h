#ifndef FORM_H
#define FORM_H

#include <QWidget>
#include <QLabel>

namespace Ui {
class Form;
}

class Form : public QWidget
{
    Q_OBJECT

public:
    explicit Form(int number, QWidget *parent = nullptr);  // Modify the constructor to accept a number
    ~Form();

private:
    Ui::Form *ui;
    int receivedNumber;  // Store the number passed from MainWindow
    QLabel *sunriselabel;
    QLabel *putsunset;
    QLabel *putsunrise;
    QLabel *labeltemperature1;
    QLabel *labeltemperature2;
    QLabel *labeltemperature3;
    QLabel *labeltemperature4;
    QLabel *labeltemperature5;
    QLabel *labeltemperature6;
    QLabel *labeltemperature7;
    QLabel *labeltemperature8;
    QLabel *labelcondition1;
    QLabel *labelcondition2;
    QLabel *labelcondition3;
    QLabel *labelcondition4;
    QLabel *labelcondition5;
    QLabel *labelcondition6;
    QLabel *labelcondition7;
    QLabel *labelcondition8;
    QLabel *labeltime1;
    QLabel *labeltime2;
    QLabel *labeltime3;
    QLabel *labeltime4;
    QLabel *labeltime5;
    QLabel *labeltime6;
    QLabel *labeltime7;
    QLabel *labeltime8;
        // Define QLabel for your GIF
};

#endif // FORM_H
