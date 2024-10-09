// form.cpp
#include "form.h"
#include "ui_form.h" // Ensure to include the generated UI header
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
#include <QMovie>
#include <QDateTime>
#include <QTime>
#include <QString>
#include <QLabel>

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
        QFile file("loc.txt");

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

                        // If the date has changed, we need to store the previous day's data
                        if (currentDay != dateStr) {
                            if (!currentDay.isEmpty()) {
                                forecastData.append(dayForecast);  // Append previous day's data
                                std::cout << "Appended forecast for: " << currentDay.toStdString() << std::endl;
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
                    forecastData.append(dayForecast); // Append the last day's data
                    std::cout << "Appended forecast for: " << currentDay.toStdString() << std::endl;
                }
            } else {
                std::cerr << "List data is missing from JSON response" << std::endl;
            }
        } else {
            std::cerr << "Sunrise or sunset data is missing from JSON response" << std::endl;
        }

        // Debugging: print the size of forecast data and the details
        std::cout << "Total forecasts retrieved: " << forecastData.size() << std::endl;
        for (const DayForecast& df : forecastData) {
            std::cout << "Date: " << df.date.toStdString()
            << ", Sunrise: " << df.sunrise.toStdString()
            << ", Sunset: " << df.sunset.toStdString() << std::endl;
        }
    }
    void Run(QWidget *parent) {
        storeFunction(parent);
        QString encodedCity = QUrl::toPercentEncoding(city);
        url = "http://api.openweathermap.org/data/2.5/forecast?q=" + encodedCity.toStdString() + "&appid=" + apiKey;

        std::string weatherData = GetWeatherData(url);
        ProcessWeatherData(weatherData);
    }
    DayForecast GetDayForecast(int dayIndex){
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
    void PrintDayForecasts( forecast& weatherForecast) {
        for (int i = 0; i < weatherForecast.forecastData.size(); ++i) {
            forecast::DayForecast df = weatherForecast.GetDayForecast(i);

            // Output the date and time slots for the current day's forecast
            std::cout << "Date: " << df.date.toStdString() << std::endl;
            std::cout << "Sunrise: " << df.sunrise.toStdString()
                      << ", Sunset: " << df.sunset.toStdString() << std::endl;

            for (const auto& timeSlot : df.timeSlots) {
                std::cout << "Time: " << timeSlot.time.toStdString()
                << ", Temperature: " << timeSlot.temperature.toStdString()
                << ", Weather: " << timeSlot.weatherCondition.toStdString() << std::endl;
            }
            std::cout << "-----------------------------------" << std::endl;
        }
    }


};

Form::Form(int number, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Form),
    receivedNumber(number)  // Initialize the received number here
{
    ui->setupUi(this);

    sunriselabel = ui->sunriselabel;

    // Set up the GIF
    QMovie *movie = new QMovie("C:/Users/Acer/OneDrive/Documents/oop project/weather images/sunset.gif");
    movie->setScaledSize(ui->sunriselabel->size());  // Scale the GIF to fit the label
    ui->sunriselabel->setMovie(movie);
    movie->start();

    const std::string apiKey = "75141d81f8a3bd990d9f95abb82f490f";
    forecast weatherForecast(apiKey);
    weatherForecast.Run(this);
    forecast::DayForecast dayforecast = weatherForecast.GetDayForecast(receivedNumber);
    weatherForecast.PrintDayForecasts(weatherForecast);

    // Set sunrise and sunset texts
    ui->putsunrise->setText(dayforecast.sunrise);
    ui->putsunset->setText(dayforecast.sunset);

    // Initialize temperature labels
    labeltemperature1 = ui->labeltemperature1;
    labeltemperature2 = ui->labeltemperature2;
    labeltemperature3 = ui->labeltemperature3;
    labeltemperature4 = ui->labeltemperature4;
    labeltemperature5 = ui->labeltemperature5;  // New label
    labeltemperature6 = ui->labeltemperature6;  // New label
    labeltemperature7 = ui->labeltemperature7;  // New label
    labeltemperature8 = ui->labeltemperature8;  // New label
    labelcondition1 = ui->labelcondition1;
    labelcondition2 = ui->labelcondition2;
    labelcondition3 = ui->labelcondition3;
    labelcondition4 = ui->labelcondition4;
    labelcondition5 = ui->labelcondition5;
    labelcondition6 = ui->labelcondition6;
    labelcondition7 = ui->labelcondition7;
    labelcondition8 = ui->labelcondition8;
    labeltime1 = ui->labeltime1;
    labeltime2 = ui->labeltime2;
    labeltime3 = ui->labeltime3;
    labeltime4 = ui->labeltime4;
    labeltime5 = ui->labeltime5;
    labeltime6 = ui->labeltime6;
    labeltime7 = ui->labeltime7;
    labeltime8 = ui->labeltime8;    // New label

    // Clear existing text to avoid mixing with previous runs
    // Clear existing text to avoid mixing with previous runs
    labeltemperature1->clear();
    labeltemperature2->clear();
    labeltemperature3->clear();
    labeltemperature4->clear();
    labeltemperature5->clear();  // Clear new labels
    labeltemperature6->clear();
    labeltemperature7->clear();
    labeltemperature8->clear();

    labelcondition1->clear();
    labelcondition2->clear();
    labelcondition3->clear();
    labelcondition4->clear();
    labelcondition5->clear();
    labelcondition6->clear();
    labelcondition7->clear();
    labelcondition8->clear();

    labeltime1->clear();
    labeltime2->clear();
    labeltime3->clear();
    labeltime4->clear();
    labeltime5->clear();
    labeltime6->clear();
    labeltime7->clear();
    labeltime8->clear();

    // Set temperatures and weather conditions based on the available time slots
    const int maxLabels = 8;  // Adjusted to handle 8 labels
    int count = 0;

    for (const auto& timeSlot : dayforecast.timeSlots) {
        if (count < maxLabels) {
            switch (count) {
            case 0:
                labeltemperature1->setText(timeSlot.temperature);
                labelcondition1->setText(timeSlot.weatherCondition);
                labeltime1->setText(timeSlot.time);                // Set condition for label 1
                break;
            case 1:
                labeltemperature2->setText(timeSlot.temperature);
                labelcondition2->setText(timeSlot.weatherCondition);
                labeltime2->setText(timeSlot.time);                // Set condition for label 2
                break;
            case 2:
                labeltemperature3->setText(timeSlot.temperature);
                labelcondition3->setText(timeSlot.weatherCondition);
                labeltime3->setText(timeSlot.time);                 // Set condition for label 3
                break;
            case 3:
                labeltemperature4->setText(timeSlot.temperature);
                labelcondition4->setText(timeSlot.weatherCondition);
                labeltime4->setText(timeSlot.time);                 // Set condition for label 4
                break;
            case 4:
                labeltemperature5->setText(timeSlot.temperature);
                labelcondition5->setText(timeSlot.weatherCondition);
                labeltime5->setText(timeSlot.time);                 // Set condition for label 5
                break;
            case 5:
                labeltemperature6->setText(timeSlot.temperature);
                labelcondition6->setText(timeSlot.weatherCondition);
                labeltime6->setText(timeSlot.time);                 // Set condition for label 6
                break;
            case 6:
                labeltemperature7->setText(timeSlot.temperature);
                labelcondition7->setText(timeSlot.weatherCondition);
                labeltime7->setText(timeSlot.time);                 // Set condition for label 7
                break;
            case 7:
                labeltemperature8->setText(timeSlot.temperature);
                labelcondition8->setText(timeSlot.weatherCondition);
                labeltime8->setText(timeSlot.time);                 // Set condition for label 8
                break;
            }
            count++;
        } else {
            break; // No more labels to fill
        }
    }
    std::cout<<receivedNumber<<std::endl;

}


Form::~Form()
{
    delete ui;  // Clean up the UI pointer
}
