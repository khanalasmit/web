#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QFile>
#include <QInputDialog>
#include <QDebug>
#include <QTextStream>
#include <iostream>
#include <windows.h>
#include <wininet.h>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QPixmap>
#include <QDateTime>
#include <QTime>
#include <QString>
#include <QLabel>
#include "form.h"

// Define WeatherApp class
class WeatherApp {
protected:
    std::string apiKey;
    std::string url;

    WeatherApp(const std::string& key) : apiKey(key) {}
    virtual ~WeatherApp() {}
};

// Define LocationStorage class
class LocationStorage {
protected:
    QString city;

public:
    void storeFunction(QWidget *parent) {
        QFile file("location.txt");

        if (!file.open(QIODevice::ReadOnly)) {
            // File does not exist or cannot be opened for reading
            if (!file.open(QIODevice::WriteOnly)) {
                qDebug() << "Failed to create file.";
                return;
            }

            // Ask the user to input the location through GUI
            city = QInputDialog::getText(parent, "Input Location", "Enter the location:");

            if (!city.isEmpty()) {
                QTextStream out(&file);
                out << city;
            }
            file.close();
        } else {
            // File exists, read the city from it
            QTextStream in(&file);
            city = in.readLine();
            file.close();

            // If the city is not present or the file is empty, ask the user to input the location
            if (city.isEmpty()) {
                city = QInputDialog::getText(parent, "Input Location", "Enter the location:");

                if (!city.isEmpty()) {
                    // Re-open file for writing and update with new city
                    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                        QTextStream out(&file);
                        out << city;
                    } else {
                        qDebug() << "Failed to open file for writing.";
                    }
                    file.close();
                }
            }
        }
    }

    QString getCity() const {
        return city;
    }
};

// Define CurrentWeather class
class CurrentWeather : public WeatherApp, public LocationStorage {
public:
    CurrentWeather(const std::string& key) : WeatherApp(key) {}

    std::string GetWeatherData(const std::string& url) {
        HINTERNET hInternet = InternetOpenW(L"WeatherApp", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) {
            std::cerr << "InternetOpenW failed" << std::endl;
            return "";
        }

        HINTERNET hConnect = InternetOpenUrlW(hInternet, std::wstring(url.begin(), url.end()).c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (!hConnect) {
            std::cerr << "InternetOpenUrlW failed" << std::endl;
            InternetCloseHandle(hInternet);
            return "";
        }

        char buffer[4096];
        DWORD bytesRead = 0;
        std::string response;

        while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            response.append(buffer, bytesRead);
        }

        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);

        return response;
    }

    void ParseAndDisplayWeatherData(const std::string& data) {
        QByteArray byteArray(data.c_str(), data.size());
        QJsonDocument doc(QJsonDocument::fromJson(byteArray));
        if (doc.isNull() || !doc.isObject()) {
            std::cerr << "Failed to parse JSON or JSON is not an object" << std::endl;
            return;
        }

        QJsonObject jsonObj = doc.object();
        QStringList weatherInfo;
        weatherInfo.append(LocationStorage::city);

        // Extract and store main weather information
        QJsonObject main = jsonObj["main"].toObject();
            //current tempreture
        if (main.contains("temp")) {
            double tempK = main["temp"].toDouble();
            double tempC = tempK - 273.15; // Convert Kelvin to Celsius
            weatherInfo.append(QString("%1 °C").arg(tempC, 0, 'f', 0));
        }else {
            weatherInfo.append("Temperature data is missing");
        }
        if (jsonObj.contains("weather")) {
            QJsonArray weatherArray = jsonObj["weather"].toArray();
            if (!weatherArray.isEmpty()) {
                QJsonObject weatherObj = weatherArray[0].toObject();
                QString description = weatherObj["description"].toString();

                // Append the weather description
                weatherInfo.append(description);

                // Append the weather condition ID as a string
                int weatherId = weatherObj["id"].toInt();
                weatherInfo.append(QString::number(weatherId));
            }
        }
            //current pressure handelling
        if (main.contains("pressure")) {
            double pressure = main["pressure"].toDouble();
            weatherInfo.append(QString("Pressure: %1 hPa").arg(pressure));
        } else {
            weatherInfo.append("Pressure data is missing");
        }
        //current humidity handellling
        if (main.contains("humidity")) {
            double humidity = main["humidity"].toDouble();
            weatherInfo.append(QString("Humidity: %1 %").arg(humidity));
        } else {
            weatherInfo.append("Humidity data is missing");
        }

        // Extract and store weather description
        if (jsonObj.contains("wind")) {
            QJsonObject wind = jsonObj["wind"].toObject();
            if (wind.contains("speed")) {
                double windSpeed = wind["speed"].toDouble()*3.6;
                weatherInfo.append(QString("%1 km/hr").arg(windSpeed));
            } else {
                weatherInfo.append("Wind speed data is missing");
            }
            //current wind direction
            if (wind.contains("deg")) {
                double windDeg = wind["deg"].toDouble();
                weatherInfo.append(QString("Wind Direction: %1 degrees").arg(windDeg));
            } else {
                weatherInfo.append("Wind direction data is missing");
            }
        } else {
            weatherInfo.append("Wind data is missing");
        }

        // Extract and store location information
        if (jsonObj.contains("name") && jsonObj.contains("sys")) {
            QString cityName = jsonObj["name"].toString();
            QString country = jsonObj["sys"].toObject()["country"].toString();
            weatherInfo.append(QString("Location: %1, %2").arg(cityName).arg(country));
        } else {
            weatherInfo.append("Location data is missing");
        }

        // Store the parsed weather data in the QStringList
        parsedWeatherData = weatherInfo;
    }

    QString GetWeatherDataString(int index) const {
        if (index >= 0 && index < parsedWeatherData.size()) {
            return parsedWeatherData[index];
        }
        return QString("Data not available");
    }

    void Run(QWidget *parent) {
        storeFunction(parent);
        url = "http://api.openweathermap.org/data/2.5/weather?q=" + city.toStdString() + "&appid=" + apiKey;
        std::string weatherData = GetWeatherData(url);
        ParseAndDisplayWeatherData(weatherData);
    }

private:
    QStringList parsedWeatherData; // To store the parsed weather data
};


class forecast : public WeatherApp, public LocationStorage {
public:
    struct TimeSlotForecast {
        QString time;
        QString temperature;
        QString weatherCondition;
    };

    struct DayForecast {
        QString date;
        QList<TimeSlotForecast> timeSlots;
        QString sunrise;
        QString sunset;
    };

    QList<DayForecast> forecastData;  // List to store day-wise forecasts

    forecast(const std::string& key) : WeatherApp(key) {}

    std::string GetWeatherData(const std::string& url) {
        HINTERNET hInternet = InternetOpenW(L"WeatherApp", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) {
            std::cerr << "InternetOpenW failed" << std::endl;
            return "";
        }

        HINTERNET hConnect = InternetOpenUrlW(hInternet, std::wstring(url.begin(), url.end()).c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (!hConnect) {
            std::cerr << "InternetOpenUrlW failed" << std::endl;
            InternetCloseHandle(hInternet);
            return "";
        }

        char buffer[4096];
        DWORD bytesRead = 0;
        std::string response;

        while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            response.append(buffer, bytesRead);
        }

        DWORD responseCode = 0;
        DWORD size = sizeof(responseCode);
        if (HttpQueryInfo(hConnect, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &responseCode, &size, NULL)) {
            std::cout << "HTTP Response Code: " << responseCode << std::endl;
        }

        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);

        return response;
    }


    void ProcessWeatherData(const std::string& weatherData) {
        QByteArray byteArray = QByteArray::fromStdString(weatherData);
        QJsonDocument doc = QJsonDocument::fromJson(byteArray);

        if (!doc.isObject()) {
            std::cerr << "Invalid JSON data" << std::endl;
            std::cerr << "Received Data: " << weatherData << std::endl;
            return;
        }

        QJsonObject jsonObj = doc.object();

        if (!jsonObj.contains("city")) {
            std::cerr << "City data is missing from JSON response" << std::endl;
            return;
        }

        QJsonObject cityObj = jsonObj["city"].toObject();
        std::cout << "City: " << cityObj["name"].toString().toStdString()
                  << ", " << cityObj["country"].toString().toStdString() << std::endl;

        if (cityObj.contains("sunrise") && cityObj.contains("sunset")) {
            qint64 sunrise = cityObj["sunrise"].toVariant().toLongLong();
            qint64 sunset = cityObj["sunset"].toVariant().toLongLong();

            QDateTime sunriseTime = QDateTime::fromSecsSinceEpoch(sunrise);
            QDateTime sunsetTime = QDateTime::fromSecsSinceEpoch(sunset);

            QString sunriseStr = sunriseTime.toString("hh:mm");
            QString sunsetStr = sunsetTime.toString("hh:mm");

            if (jsonObj.contains("list")) {
                QJsonArray listArray = jsonObj["list"].toArray();
                QString currentDay = "";  // To track and group data by day
                DayForecast dayForecast;

                for (int i = 0; i < listArray.size(); ++i) {
                    QJsonObject item = listArray[i].toObject();

                    if (item.contains("dt")) {
                        qint64 dt = item["dt"].toVariant().toLongLong();
                        QDateTime dateTime = QDateTime::fromSecsSinceEpoch(dt);

                        QString dateStr = dateTime.toString("dddd, d MMMM");  // Format: Sunday, 9 September
                        QString timeStr = dateTime.toString("hh:mm");         // Format: 15:00

                        if (currentDay != dateStr) {
                            if (!currentDay.isEmpty()) {
                                forecastData.append(dayForecast);  // Append previous day's data
                            }
                            dayForecast = DayForecast();  // Reset dayForecast for a new day
                            dayForecast.date = dateStr;
                            dayForecast.sunrise = sunriseStr;
                            dayForecast.sunset = sunsetStr;
                            currentDay = dateStr;
                        }

                        QString temperatureStr;
                        if (item.contains("main")) {
                            QJsonObject mainObj = item["main"].toObject();
                            double tempK = mainObj["temp"].toDouble();
                            double tempC = tempK - 273.15;  // Convert Kelvin to Celsius
                            temperatureStr = QString("%1°C").arg(tempC, 0, 'f', 0);
                        }

                        QString weatherCondition;
                        if (item.contains("weather")) {
                            QJsonArray weatherArray = item["weather"].toArray();
                            if (!weatherArray.isEmpty()) {
                                QJsonObject weatherObj = weatherArray[0].toObject();
                                weatherCondition = weatherObj["description"].toString();
                            }
                        }

                        TimeSlotForecast timeSlot;
                        timeSlot.time = timeStr;
                        timeSlot.temperature = temperatureStr;
                        timeSlot.weatherCondition = weatherCondition;
                        dayForecast.timeSlots.append(timeSlot);
                    }
                }

                if (!currentDay.isEmpty()) {
                    forecastData.append(dayForecast);
                }
            } else {
                std::cerr << "List data is missing from JSON response" << std::endl;
            }
        } else {
            std::cerr << "Sunrise or sunset data is missing from JSON response" << std::endl;
        }
    }


    void Run(QWidget *parent) {
        storeFunction(parent);
        QString encodedCity = QUrl::toPercentEncoding(city);
        url = "http://api.openweathermap.org/data/2.5/forecast?q=" + encodedCity.toStdString() + "&appid=" + apiKey;

        std::string weatherData = GetWeatherData(url);
        ProcessWeatherData(weatherData);
    }

    // Method to retrieve the date and time slot forecasts for a specific day
    DayForecast GetDayForecast(int dayIndex) const {
        if (dayIndex >= 0 && dayIndex < forecastData.size()) {
            const DayForecast& dayForecast = forecastData[dayIndex];
            // Create a copy of dayForecast to return
            DayForecast resultForecast = dayForecast;

            // Return the original date without formatting
            return resultForecast;
        }

        // Return an empty DayForecast structure if no valid index
        return DayForecast();
    }



    // Method to retrieve forecasts for the current day only
    QString GetTodayForecast() const {
        QDateTime now = QDateTime::currentDateTime();
        QString todayDate = now.toString("dddd, d MMMM");

        for (const DayForecast& dayForecast : forecastData) {
            if (dayForecast.date == todayDate) {
                QString result = QString("Date: %1\nSunrise: %2\nSunset: %3\n\n").arg(dayForecast.date, dayForecast.sunrise, dayForecast.sunset);

                for (const TimeSlotForecast& slot : dayForecast.timeSlots) {
                    result += QString("Time: %1, Temperature: %2, Condition: %3\n")
                    .arg(slot.time, slot.temperature, slot.weatherCondition);
                }
                return result;
            }
        }
        return QString("No Data for Today");
    }
    QList<TimeSlotForecast> GetRemainingTimeForecasts() const {
        QList<TimeSlotForecast> remainingForecasts;
        QString todayDate = QDateTime::currentDateTime().toString("dddd, d MMMM");

        for (const DayForecast& dayForecast : forecastData) {
            if (dayForecast.date == todayDate) {
                QDateTime now = QDateTime::currentDateTime();
                for (const TimeSlotForecast& slot : dayForecast.timeSlots) {
                    QDateTime slotDateTime = QDateTime::fromString(slot.time, "hh:mm");
                    if (now.time() <= slotDateTime.time()) {
                        remainingForecasts.append(slot);
                    }
                }
                break;  // Only need today's data
            }
        }

        return remainingForecasts;
    }
};




// MainWindow constructor
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , form(nullptr)
{
    ui->setupUi(this);
    firstdisplay = ui->firstdisplay;
    condition =ui->condition;
    date=ui->date;
    humidity=ui->humidity;
    currentlocation=ui->currentlocation;
    currenttemperature=ui->currenttemperature;
    currentcondition=ui->currentcondition;
    currentwind=ui->currentwind;
    currentpressure=ui->currentpressure;
    currentweathericon=ui->currentweathericon;

    labelTime1=ui->labeltime1;
    labeltemperature1=ui->labeltemperature1;
    labelcondition1=ui->labelCondition1;
    labelTime2=ui->labeltime2;
    labeltemperature2=ui->labeltemperature2;
    labelcondition2=ui->labelCondition2;
    labelTime3=ui->labeltime3;
    labeltemperature3=ui->labeltemperature3;
    labelcondition3=ui->labelCondition3;
    labelTime4=ui->labeltime4;
    labeltemperature4=ui->labeltemperature4;
    labelcondition4=ui->labelCondition4;
    labelTime5=ui->labeltime5;
    labeltemperature5=ui->labeltemperature5;
    labelcondition5=ui->labelCondition5;
    labelTime6=ui->labeltime6;
    labeltemperature6=ui->labeltemperature6;
    labelcondition6=ui->labelCondition6;
    labelTime7=ui->labeltime7;
    labeltemperature7=ui->labeltemperature7;
    labelcondition7=ui->labelCondition7;
    connect(ui->InitialButton, &QPushButton::clicked, this, &MainWindow::on_InitialButton_clicked);
    connect(ui->QuitButton, &QPushButton::clicked, this, &MainWindow::on_QuitButton_clicked);
    connect(ui->GOTOMAINWINDOW, &QPushButton::clicked, this, &MainWindow::on_GOTOMAINWINDOW_clicked);
    ui->currentlocation->setVisible(false);
    ui->currentcondition->setVisible(false);
    ui->QuitButton->setVisible(false);
    ui->currentwind->setVisible(false);
    ui->currentpressure->setVisible(false);
    ui->currentweathericon->setVisible(false);

    ui->firstdisplay->setVisible(true);
    ui->date->setVisible(true);
    ui->condition->setVisible(true);
    ui->humidity->setVisible(true);
    ui->labeltime1->setVisible(false);
    ui->labeltemperature1->setVisible(false);
    ui->labelCondition1->setVisible(false);

    ui->labeltime2->setVisible(false);
    ui->labeltemperature2->setVisible(false);
    ui->labelCondition2->setVisible(false);

    ui->labeltime3->setVisible(false);
    ui->labeltemperature3->setVisible(false);
    ui->labelCondition3->setVisible(false);

    ui->labeltime4->setVisible(false);
    ui->labeltemperature4->setVisible(false);
    ui->labelCondition4->setVisible(false);

    ui->labeltime5->setVisible(false);
    ui->labeltemperature5->setVisible(false);
    ui->labelCondition5->setVisible(false);

    ui->labeltime6->setVisible(false);
    ui->labeltemperature6->setVisible(false);
    ui->labelCondition6->setVisible(false);

    ui->labeltime7->setVisible(false);
    ui->labeltemperature7->setVisible(false);
    ui->labelCondition7->setVisible(false);

    ui->pushButtonday1->setVisible(false);
    ui->pushButtonday2->setVisible(false);
    ui->pushButtonday3->setVisible(false);
    ui->pushButtonday4->setVisible(false);

}

// MainWindow destructor
MainWindow::~MainWindow()
{
    delete ui;
}

// Initial button click handler
void MainWindow::on_InitialButton_clicked()
{
    QMessageBox::about(this, "Weather App Info", "This is the weather forecaster application developed\n "
                                                 "as a C++ project.\n creators:\nASMIT KHANAL\nKUSHAL REGMI\nKRISH BANSAL");
    ui->QuitButton->setVisible(true);
    ui->InitialButton->setVisible(false);
}

// Quit button click handler
void MainWindow::on_QuitButton_clicked()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Quit", "Are you sure you want to quit?",
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        qApp->quit();  // Quit the application if Yes is pressed
    }
}

// Go to main window button click handler
void MainWindow::on_GOTOMAINWINDOW_clicked()
{//current weather lable settext
    ui->GOTOMAINWINDOW->setVisible(false);
    const std::string apiKey = "75141d81f8a3bd990d9f95abb82f490f";  // Replace with your actual API key
    CurrentWeather weatherApp(apiKey);
    weatherApp.Run(this);
    QString weatherData = weatherApp.GetWeatherDataString((0));
    ui->currentlocation->setText(weatherData);
    weatherData=weatherApp.GetWeatherDataString(1);
    ui->currenttemperature->setText(weatherData);
    weatherData=weatherApp.GetWeatherDataString(4);
    ui->currentpressure->setText(weatherData);
    weatherData=weatherApp.GetWeatherDataString(2);
    ui->currentcondition->setText(weatherData);
    weatherData=weatherApp.GetWeatherDataString(6);
    ui->currentwind->setText(weatherData);
    QString weatheridstr=weatherApp.GetWeatherDataString(3);
    int weatherId = weatheridstr.toInt();
    if (weatherId >= 200 && weatherId < 300) {
        ui->currentweathericon->setPixmap(QPixmap("C:/Users/Acer/OneDrive/Documents/oop project/weather images/thunderstrom.png"));
    } else if (weatherId >= 500 && weatherId < 521) {
        ui->currentweathericon->setPixmap(QPixmap("C:/Users/Acer/OneDrive/Documents/oop project/weather images/rain.png"));
    }else if (weatherId >= 521 && weatherId < 600) {
        ui->currentweathericon->setPixmap(QPixmap("C:/Users/Acer/OneDrive/Documents/oop project/weather images/shower rain.png"));
    }else if (weatherId ==800) {
        ui->currentweathericon->setPixmap(QPixmap("C:/Users/Acer/OneDrive/Documents/oop project/weather images/sunnyday.png"));
    } else if (weatherId ==801) {
        ui->currentweathericon->setPixmap(QPixmap("C:/Users/Acer/OneDrive/Documents/oop project/weather images/few clouds.png"));
    } else if (weatherId ==802) {
        ui->currentweathericon->setPixmap(QPixmap("C:/Users/Acer/OneDrive/Documents/oop project/weather images/scattered clouds.png"));
    }else if (weatherId ==803) {
        ui->currentweathericon->setPixmap(QPixmap("C:/Users/Acer/OneDrive/Documents/oop project/weather images/broken clouds.png"));
    }else if (weatherId ==804) {
        ui->currentweathericon->setPixmap(QPixmap("C:/Users/Acer/OneDrive/Documents/oop project/weather images/overcast clouds.png"));
    } else {
        ui->currentweathericon->setPixmap(QPixmap("C:/Users/Acer/OneDrive/Documents/oop project/weather images"));
    }
    ui->currentweathericon->setVisible(true);

//the current weather gui are made visible below

    ui->currentlocation->setVisible(true);
    ui->QuitButton->setVisible(true);
    ui->GOTOMAINWINDOW->setVisible(false);
    ui->firstdisplay->setVisible(false);
    ui->condition->setVisible(false);
    ui->date->setVisible(false);
    ui->humidity->setVisible(false);
    ui->InitialButton->setVisible(false);
    ui->currentcondition->setVisible(true);
    ui->currentwind->setVisible(true);
    ui->currentpressure->setVisible(true);
//below here the gui of the weather forecast is set
    forecast weatherForecast(apiKey);
    weatherForecast.Run(this);
    QList<forecast::TimeSlotForecast> remainingForecasts = weatherForecast.GetRemainingTimeForecasts();
   int maxLabels = remainingForecasts.size() > 7 ? 7 : remainingForecasts.size();
    for (int i = 0; i < maxLabels; ++i) {
        const auto& slot = remainingForecasts[i];
        switch (i) {
        case 0:
            ui->labeltime1->setText(slot.time);
            ui->labeltemperature1->setText(slot.temperature);
            ui->labelCondition1->setText(slot.weatherCondition);
            ui->labeltime1->setVisible(true);
            ui->labeltemperature1->setVisible(true);
            ui->labelCondition1->setVisible(true);
            break;
        case 1:
            ui->labeltime2->setText(slot.time);
            ui->labeltemperature2->setText(slot.temperature);
            ui->labelCondition2->setText(slot.weatherCondition);
            ui->labeltime2->setVisible(true);
            ui->labeltemperature2->setVisible(true);
            ui->labelCondition2->setVisible(true);
        case 2:
            ui->labeltime3->setText(slot.time);
            ui->labeltemperature3->setText(slot.temperature);
            ui->labelCondition3->setText(slot.weatherCondition);
            ui->labeltime3->setVisible(true);
            ui->labeltemperature3->setVisible(true);
            ui->labelCondition3->setVisible(true);
            break;
        case 3:
            ui->labeltime4->setText(slot.time);
            ui->labeltemperature4->setText(slot.temperature);
            ui->labelCondition4->setText(slot.weatherCondition);
            ui->labeltime4->setVisible(true);
            ui->labeltemperature4->setVisible(true);
            ui->labelCondition4->setVisible(true);
            break;
        case 4:
            ui->labeltime5->setText(slot.time);
            ui->labeltemperature5->setText(slot.temperature);
            ui->labelCondition5->setText(slot.weatherCondition);
            ui->labeltime5->setVisible(true);
            ui->labeltemperature5->setVisible(true);
            ui->labelCondition5->setVisible(true);
            break;
        case 5:
            ui->labeltime6->setText(slot.time);
            ui->labeltemperature6->setText(slot.temperature);
            ui->labelCondition6->setText(slot.weatherCondition);
            ui->labeltime6->setVisible(true);
            ui->labeltemperature6->setVisible(true);
            ui->labelCondition6->setVisible(true);
            break;
        case 6:
            ui->labeltime7->setText(slot.time);
            ui->labeltemperature7->setText(slot.temperature);
            ui->labelCondition7->setText(slot.weatherCondition);
            ui->labeltime7->setVisible(true);
            ui->labeltemperature7->setVisible(true);
            ui->labelCondition7->setVisible(true);
            break;


        }
    }
    for(int i = 1; i <= 4; i++) {
        forecast::DayForecast forecast = weatherForecast.GetDayForecast(i);
        std::string dateStdString = forecast.date.toStdString();
        QString dateQString = QString::fromStdString(dateStdString);

        switch(i) {
        case 1:
            ui->pushButtonday1->setText(dateQString);
            ui->pushButtonday1->setVisible(true);
            break;
        case 2:
            ui->pushButtonday2->setText(dateQString);
            ui->pushButtonday2->setVisible(true);
            break;
        case 3:
            ui->pushButtonday3->setText(dateQString);
            ui->pushButtonday3->setVisible(true);
            break;
        case 4:
            ui->pushButtonday4->setText(dateQString);
            ui->pushButtonday4->setVisible(true);
            break;
        }
    } 

}

void MainWindow::on_pushButtonday1_clicked()
{
    form=new Form(1);
    form->show();
}


void MainWindow::on_pushButtonday2_clicked()
{
    form=new Form(2);
    form->show();

}


void MainWindow::on_pushButtonday3_clicked()
{
    form=new Form(3);
    form->show();

}


void MainWindow::on_pushButtonday4_clicked()
{
    form=new Form(4);
    form->show();

}

