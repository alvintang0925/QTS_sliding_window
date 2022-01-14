//
//  main.cpp
//  QTS Algorithm
//
//  Created by 唐健恆 on 2021/8/11.
//  Copyright © 2021 Alvin. All rights reserved.
//

#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <string>
#include <vector>
#include <cstdlib>
#include <iomanip>
#include <cmath>
#include <ctime>
#include <cfloat>
#include "date.h"
#include "portfolio.h"

using namespace std;
using namespace filesystem;

#define EXPNUMBER 1
#define ITERNUMBER 100
#define PARTICLENUMBER 10
#define FUNDS 10000000.0
#define QTSTYPE 2 //QTS 0, GQTS 1, GNQTS 2, ANGQTS 3
#define TRENDLINETYPE 0 //linear 0, quadratic 1
#define MODE "train" //train, test, single

double DELTA = 0.0004;
double LOWER = 0.00045;
double UPPER = 0.00125;

string FILE_DIR = "12290_0.0004_GN_LN";
string DATA_FILE_DIR = "DJI_30";


string* vectorToArray(vector<string>& data_vector) {
    string* d = new string[data_vector.size()];
    for (int j = 0; j < data_vector.size(); j++) {
        d[j] = data_vector[j];
    }
    return d;
}

string** vectorToArray(vector<vector<string>>& data_vector) {
    string** d = new string * [data_vector.size()];
    for (int j = 0; j < data_vector.size(); j++) {
        d[j] = vectorToArray(data_vector[j]);
    }
    return d;
}

string*** vectorToArray(vector<vector<vector<string>>>& data_vector) {
    string*** d = new string * *[data_vector.size()];
    for (int j = 0; j < data_vector.size(); j++) {
        d[j] = vectorToArray(data_vector[j]);
    }
    return d;
}

bool readData(string filename, vector<vector<string>>& data_vector, int& size, int& day_number) {
    cout << filename << endl;
    ifstream inFile(filename, ios::in);
    string line;
    vector< vector<string> > temp;

    if (!inFile) {
        cout << "Open file failed!" << endl;
        exit(1);
    }
    while (getline(inFile, line)) {
        istringstream delime(line);
        string s;
        vector<string> line_data;
        while (getline(delime, s, ',')) {
            if (s != "\r") {
                s.erase(remove(s.begin(), s.end(), '\r'), s.end());
                line_data.push_back(s);
            }
        }
        temp.push_back(line_data);
    }
    inFile.close();

    size = temp[0].size() - 1;
    day_number = temp.size() - 1;
    data_vector = temp;

    return true;
}

bool readSpeData(string filename, vector<string>& data_vector, int& size, string title) {
    cout << filename << endl;
    ifstream inFile(filename, ios::in);
    string line;
    vector< vector<string> > data;

    if (!inFile) {
        cout << "Open file failed!" << endl;
        return false;
    }
    bool sw = false;
    vector<string> line_data;
    while (getline(inFile, line)) {
        istringstream delime(line);
        string s;

        while (getline(delime, s, ',')) {
            if (sw) {
                //s.pop_back();
                if (s != "\r") {
                    s.erase(remove(s.begin(), s.end(), '\r'), s.end());
                    line_data.push_back(s);
                }
                //sw = false;
            }
            if (s == title) {
                sw = true;
            }
        }
        sw = false;
    }
    inFile.close();
    size = line_data.size();
    data_vector = line_data;

    return true;
}

void createStock(Stock* stock_list, int size, int day_number, string** data) {
    for (int j = 0; j < size; j++) {
        stock_list[j].idx = j;
        stock_list[j].init(day_number);
        stock_list[j].company_name = data[0][j];
        for (int k = 1; k < day_number + 1; k++) {
            stock_list[j].price_list[k - 1] = atof(data[k][j].c_str());
        }
    }
}

void createStock(Stock* stock_list, int size, int day_number, string** data, int start_index, int end_index) {
    for (int j = 0; j < size; j++) {
        stock_list[j].idx = j;
        stock_list[j].init(day_number + 1);
        stock_list[j].company_name = data[0][j+1];
        for (int k = start_index - 1; k <= end_index; k++) {
            stock_list[j].price_list[k - start_index + 1] = atof(data[k][j + 1].c_str());
            stock_list[j].date_list[k - start_index + 1] = data[k][0];
        }
    }
}

void outputFile(Portfolio& portfolio, string file_name) {
    ofstream outfile;
    outfile.open(file_name, ios::out);
    outfile << setprecision(15);

    outfile << "Period," << portfolio.date_list[0] << "-" << portfolio.date_list[portfolio.day_number - 1] << endl;
    outfile << "Total days," << portfolio.day_number << endl;
    outfile << "Iteration," << ITERNUMBER << endl;
    outfile << "Element number," << PARTICLENUMBER << endl;
    outfile << "Delta," << DELTA << endl;
    outfile << "Exp times," << EXPNUMBER << endl << endl;

    outfile << "Init funds," << portfolio.funds << endl;
    outfile << "Final funds," << portfolio.total_money[portfolio.day_number - 1] << endl;
    outfile << "Real award," << portfolio.total_money[portfolio.day_number - 1] - portfolio.funds << endl << endl;

    outfile << "MMD," << portfolio.MDD << endl;
    outfile << "DPF," << portfolio.DPF << endl << endl;

    outfile << "m," << portfolio.m << endl;
    outfile << "Daily_risk," << portfolio.daily_risk << endl;
    outfile << "Trend," << portfolio.trend << endl << endl;

    if (TRENDLINETYPE == 0) {
        portfolio.countQuadraticYLine();
        double sum = 0;
        for (int k = 0; k < portfolio.day_number; k++) {
            double Y;
            Y = portfolio.getQuadraticY(k + 1);
            sum += (portfolio.total_money[k] - Y) * (portfolio.total_money[k] - Y);
        }
        double c = (portfolio.getQuadraticY(portfolio.day_number) - portfolio.getQuadraticY(1)) / (portfolio.day_number - 1);
        double d = sqrt(sum / (portfolio.day_number));

        outfile << "Quadratic trend line," << portfolio.a << "x^2 + " << portfolio.b << "x + " << FUNDS << endl << endl;
        outfile << "Quadratic m," << c << endl;
        outfile << "Quadratic daily risk," << d << endl;
        if (c < 0) {
            outfile << "Quadratic trend," << c * d << endl << endl;
        }
        else {
            outfile << "Quadratic trend," << c / d << endl << endl;
        }
    }
    else {
        outfile << "Quadratic trend line," << portfolio.a << "x^2 + " << portfolio.b << "x + " << FUNDS << endl << endl;
        double x = 0;
        double y = 1;
        double sum = 0;
        for (int k = 0; k < portfolio.day_number - 1; k++) {
            x += (k + 2) * (portfolio.total_money[k + 1] - portfolio.funds);
            y += (k + 2) * (k + 2);
        }

        double c = x / y;
        for (int k = 0; k < portfolio.day_number; k++) {
            double Y;
            Y = c * (k + 1) + portfolio.funds;
            sum += (portfolio.total_money[k] - Y) * (portfolio.total_money[k] - Y);
        }
        double d = sqrt(sum / (portfolio.day_number));

        outfile << "Linear m," << c << endl;
        outfile << "Linear daily risk," << d << endl;
        if (c < 0) {
            outfile << "Linear trend," << c * d << endl << endl;
        }
        else {
            outfile << "Linear trend," << c / d << endl << endl;
        }
    }

    outfile << "Best generation," << portfolio.gen << endl;
    outfile << "Best experiment," << portfolio.exp << endl;
    outfile << "Best answer times," << portfolio.answer_counter << endl << endl;

    outfile << "Stock number," << portfolio.stock_number << endl;
    outfile << "Stock#,";
    for (int j = 0; j < portfolio.stock_number; j++) {
        outfile << portfolio.constituent_stocks[portfolio.stock_id_list[j]].company_name << ",";
    }
    outfile << endl;

    outfile << "Number,";
    for (int j = 0; j < portfolio.stock_number; j++) {
        outfile << portfolio.investment_number[j] << ",";
    }
    outfile << endl;

    outfile << "Distribute funds,";
    for (int j = 0; j < portfolio.stock_number; j++) {
        outfile << portfolio.getDMoney() << ",";
    }
    outfile << endl;

    outfile << "Remain funds,";
    for (int j = 0; j < portfolio.stock_number; j++) {
        outfile << portfolio.remain_fund[j] << ",";
    }
    outfile << endl;

    outfile << "Date,";
    for (int j = 0; j < portfolio.stock_number; j++) {
        outfile << ",";
    }
    outfile << "FS" << endl;

    for (int j = 0; j < portfolio.day_number; j++) {
        outfile << portfolio.date_list[j] << ",";
        for (int k = 0; k < portfolio.stock_number; k++) {
            outfile << (portfolio.constituent_stocks[portfolio.stock_id_list[k]].price_list[j + 1] * portfolio.investment_number[k]) + portfolio.remain_fund[k] << ",";
        }
        outfile << portfolio.total_money[j] << endl;
    }
    outfile << endl;
    outfile.close();
}

void recordCPUTime(double START, double END, string file_name) {
    double total_time = (END - START) / CLOCKS_PER_SEC;
    ofstream outfile_time;
    outfile_time.open(file_name, ios::out);
    outfile_time << "total time: " << total_time << " sec" << endl;
    outfile_time.close();
}


void recordDetail(ofstream& outfile_detail, Portfolio& result) {

    outfile_detail << result.date << ",";
    outfile_detail << result.getProfit() << ",";
    outfile_detail << result.m << ",";
    outfile_detail << result.daily_risk << ",";
    outfile_detail << result.trend << ",";
    outfile_detail << result.MDD << ",";
    outfile_detail << result.DPF << ",";
    outfile_detail << result.mood_ratio << ",";
    outfile_detail << result.stock_number << ",";
    for (int j = 0; j < result.stock_number; j++) {
        outfile_detail << result.constituent_stocks[result.stock_id_list[j]].company_name << " ";
    }
    outfile_detail << endl;
}

void recordTotalResult(ofstream& outfile_total_data, Portfolio& result, Date current_date) {
    outfile_total_data << current_date.getDate() + " - " + current_date.getRangeEnd(current_date.train_range - 1).getDate() << ",";
    outfile_total_data << result.date << ",";
    outfile_total_data << result.day_number << ",";
    outfile_total_data << result.MDD << ",";
    outfile_total_data << result.getProfit() << ",";
    outfile_total_data << result.DPF << ",";

    if (TRENDLINETYPE == 0) {
        outfile_total_data << result.trend << ",";
        double sum = 0;
        for (int k = 0; k < result.day_number; k++) {
            double Y;
            Y = result.getQuadraticY(k + 1);
            sum += (result.total_money[k] - Y) * (result.total_money[k] - Y);
        }
        double c = (result.getQuadraticY(result.day_number) - result.getQuadraticY(1)) / (result.day_number - 1);
        double d = sqrt(sum / (result.day_number));


        if (c < 0) {
            outfile_total_data << c * d << ",";
        }
        else {
            outfile_total_data << c / d << ",";
        }

    }
    else {
        double x = 0;
        double y = 1;
        double sum = 0;
        for (int k = 0; k < result.day_number - 1; k++) {
            x += (k + 2) * (result.total_money[k + 1] - result.funds);
            y += (k + 2) * (k + 2);
        }

        double c = x / y;
        for (int k = 0; k < result.day_number; k++) {
            double Y;
            Y = c * (k + 1) + result.funds;
            sum += (result.total_money[k] - Y) * (result.total_money[k] - Y);
        }
        double d = sqrt(sum / (result.day_number));

        if (c < 0) {
            outfile_total_data << c * d << ",";
        }
        else {
            outfile_total_data << c / d << ",";
        }
        outfile_total_data << result.trend << ",";
    }
    
    outfile_total_data << result.mood_ratio << ",";
    outfile_total_data << 0 << ",";
    outfile_total_data << result.stock_number << ",";
    for (int j = 0; j < result.stock_number; j++) {
        outfile_total_data << result.constituent_stocks[result.stock_id_list[j]].company_name << " ";
    }
    outfile_total_data << endl;
}



void createDir(string file_dir, string type, string mode) {
    create_directory(file_dir);
    create_directory(file_dir + "/" + type);
    create_directory(file_dir + "/" + type +"/" + mode);
}

void createDir(string file_dir, string type, string mode, string company_name) {
    createDir(file_dir, type, mode);
    create_directory(file_dir + "/" + type + "/" + mode + "/" + company_name);

}

void preSet(string mode, Date& current_date, Date& finish_date, int SLIDETYPE, string& TYPE) {
    string STARTYEAR;
    string STARTMONTH;
    string ENDYEAR;
    string ENDMONTH;
    int slide_number;
    int train_range;

    switch (SLIDETYPE) {
    case 0:
        STARTYEAR = "2010";
        STARTMONTH = "12";
        ENDYEAR = "2020";
        ENDMONTH = "11";
        TYPE = "M2M";
        train_range = 1;
        slide_number = 1;
        break;
    case 1:
        STARTYEAR = "2010";
        STARTMONTH = "10";
        ENDYEAR = "2020";
        ENDMONTH = "9";
        TYPE = "Q2M";
        train_range = 3;
        slide_number = 1;
        break;
    case 2:
        STARTYEAR = "2010";
        STARTMONTH = "10";
        ENDYEAR = "2020";
        ENDMONTH = "7";
        TYPE = "Q2Q";
        train_range = 3;
        slide_number = 3;
        break;
    case 3:
        STARTYEAR = "2010";
        STARTMONTH = "7";
        ENDYEAR = "2020";
        ENDMONTH = "6";
        TYPE = "H2M";
        train_range = 6;
        slide_number = 1;
        break;
    case 4:
        STARTYEAR = "2010";
        STARTMONTH = "7";
        ENDYEAR = "2020";
        ENDMONTH = "4";
        TYPE = "H2Q";
        train_range = 6;
        slide_number = 3;
        break;
    case 5:
        STARTYEAR = "2010";
        STARTMONTH = "7";
        ENDYEAR = "2020";
        ENDMONTH = "1";
        TYPE = "H2H";
        train_range = 6;
        slide_number = 6;
        break;
    case 6:
        STARTYEAR = "2010";
        STARTMONTH = "1";
        ENDYEAR = "2019";
        ENDMONTH = "12";
        TYPE = "Y2M";
        train_range = 12;
        slide_number = 1;
        break;
    case 7:
        STARTYEAR = "2010";
        STARTMONTH = "1";
        ENDYEAR = "2019";
        ENDMONTH = "10";
        TYPE = "Y2Q";
        train_range = 12;
        slide_number = 3;
        break;
    case 8:
        STARTYEAR = "2010";
        STARTMONTH = "1";
        ENDYEAR = "2019";
        ENDMONTH = "7";
        TYPE = "Y2H";
        train_range = 12;
        slide_number = 6;
        break;
    case 9:
        STARTYEAR = "2010";
        STARTMONTH = "1";
        ENDYEAR = "2019";
        ENDMONTH = "1";
        TYPE = "Y2Y";
        train_range = 12;
        slide_number = 12;
        break;
    case 10:
        STARTYEAR = "2010";
        STARTMONTH = "1";
        ENDYEAR = "2019";
        ENDMONTH = "12";
        TYPE = "M#";
        if (mode == "test") {
            train_range = 12;
        }
        else {
            train_range = 1;
        }
        slide_number = 1;
        break;
    case 11:
        STARTYEAR = "2010";
        STARTMONTH = "1";
        ENDYEAR = "2019";
        ENDMONTH = "10";
        TYPE = "Q#";
        if (mode == "test") {
            train_range = 12;
        }
        else {
            train_range = 3;
        }
        slide_number = 3;
        break;
    case 12:
        STARTYEAR = "2010";
        STARTMONTH = "1";
        ENDYEAR = "2019";
        ENDMONTH = "7";
        TYPE = "H#";
        if (mode == "test") {
            train_range = 12;
        }
        else {
            train_range = 6;
        }
        slide_number = 6;
        break;
    }

    if (TRENDLINETYPE == 0) {
        TYPE += "_LN";
    }
    else if (TRENDLINETYPE == 1) {
        TYPE += "_QD";
    }

    current_date.date.tm_year = atoi(STARTYEAR.c_str()) - 1900;
    current_date.date.tm_mon = atoi(STARTMONTH.c_str()) - 1;
    current_date.date.tm_mday = 1;
    current_date.slide_number = slide_number;
    current_date.train_range = train_range;

    finish_date.date.tm_year = atoi(ENDYEAR.c_str()) - 1900;
    finish_date.date.tm_mon = atoi(ENDMONTH.c_str()) - 1;
    finish_date.date.tm_mday = 1;
    finish_date.slide_number = slide_number;
    finish_date.train_range = train_range;

    if (mode == "test") {
        current_date.slide(train_range);
        finish_date.slide(train_range);
    }
}

void setWindow(string mode, string& start_date, string& end_date, int& start_index, int& end_index, Date current_date, Date finish_data, string* data_copy, string** data, int day_number, int& range_day_number) {
    bool sw = false;//判斷是否找到開始日期
    int flag = 0;//判斷是否找到結束月份
    string temp1 = current_date.getYear() + current_date.getMon();
    string temp2;
    if (mode == "test") {
        temp2 = current_date.getRangeEnd(current_date.slide_number - 1).getYear() + current_date.getRangeEnd(current_date.slide_number - 1).getMon();
    }
    else {
        temp2 = current_date.getRangeEnd(current_date.train_range - 1).getYear() + current_date.getRangeEnd(current_date.train_range - 1).getMon();
    }

    for (int j = 0; j < day_number; j++) {

        if (!sw && temp1 == data_copy[j]) {
            start_date = data[j + 1][0];
            start_index = j + 1;
            sw = true;
        }

        if (sw) {
            if (flag == 1 && temp2 != data_copy[j]) {
                end_date = data[j][0];
                end_index = j;
                flag = 0;
                break;
            }
            if (flag == 0 && temp2 == data_copy[j]) {
                flag = 1;
            }
        }
    }

    if (flag == 1) {
        end_date = data[day_number][0];
        end_index = day_number;
    }
    range_day_number = end_index - start_index + 1;
}

void copyData(string* data_copy, string** data, int day_number) {
    for (int j = 0; j < day_number; j++) {
        data_copy[j] = data[j + 1][0];
        data_copy[j].resize(6);
    }
}

void releaseData(vector<vector<string>>& price_data_vector, vector<vector<vector<string>>>& RSI_data_vector, string** price_data, string*** RSI_data, string* data_copy) {
    for (int j = 0; j < price_data_vector.size(); j++) {
        delete[] price_data[j];
        price_data_vector[j].clear();
    }
    delete[] price_data;
    price_data_vector.clear();

    for (int j = 0; j < RSI_data_vector.size(); j++) {
        for (int k = 0; k < RSI_data_vector[j].size(); k++) {
            delete[] RSI_data[j][k];
            RSI_data_vector[j][k].clear();
        }
        delete[] RSI_data[j];
        RSI_data_vector[j].clear();
    }

    delete[] RSI_data;
    RSI_data_vector.clear();
    delete[] data_copy;
}

string getPriceFilename(Date current_date, string mode, int SLIDETYPE, string TYPE) {
    if (mode != "test") {
        mode = "train";
    }

    Date temp;
    if (mode == "train") {
        switch (SLIDETYPE) {
        case 0://M2M
            TYPE = "M2M";
            return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "(" + current_date.getYear() + " Q1).csv";
            break;
        case 1://Q2M
            TYPE = "Q2M";
            temp = current_date.getRangeEnd(current_date.train_range - 1);
            if (current_date.getYear() != temp.getYear()) {
                return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "~" + temp.getYear() + "_" + temp.getMon() + "(" + current_date.getYear() + " Q1).csv";
            }
            else {
                return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "-" + temp.getMon() + "(" + current_date.getYear() + " Q1).csv";
            }
            break;
        case 2://Q2Q
            TYPE = "Q2Q";
            return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "(" + current_date.getYear() + " Q1).csv";
            break;
        case 3://H2M
            TYPE = "H2M";
            temp = current_date.getRangeEnd(current_date.train_range - 1);
            if (current_date.getYear() != temp.getYear()) {
                return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "~" + temp.getYear() + "_" + temp.getMon() + "(" + current_date.getYear() + " Q1).csv";
            }
            else {
                return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "-" + temp.getMon() + "(" + current_date.getYear() + " Q1).csv";
            }
            break;
        case 4://H2Q
            TYPE = "H2Q";
            temp = current_date.getRangeEnd(current_date.train_range - 1);
            if (current_date.getYear() != temp.getYear()) {
                return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "~" + temp.getYear() + "_" + temp.getQ() + "(" + current_date.getYear() + " Q1).csv";
            }
            else {
                return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "-" + temp.getQ() + "(" + current_date.getYear() + " Q1).csv";
            }
            break;
        case 5://H2H
            TYPE = "H2H";
            temp = current_date.getRangeEnd(current_date.train_range - 1);
            return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "-" + temp.getQ() + "(" + current_date.getYear() + " Q1).csv";
            break;
        case 6://Y2M
            TYPE = "Y2M";
            temp = current_date.getRangeEnd(current_date.train_range - 1);
            if (current_date.getYear() != temp.getYear()) {
                return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "~" + temp.getYear() + "_" + temp.getMon() + "(" + current_date.getYear() + " Q1).csv";
            }
            else {
                return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "(" + current_date.getYear() + " Q1).csv";
            }
            break;
        case 7://Y2Q
            TYPE = "Y2Q";
            temp = current_date.getRangeEnd(current_date.train_range - 1);
            if (current_date.getYear() != temp.getYear()) {
                return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "~" + temp.getYear() + "_" + temp.getQ() + "(" + current_date.getYear() + " Q1).csv";
            }
            else {
                return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "(" + current_date.getYear() + " Q1).csv";
            }
            break;
        case 8://Y2H
            TYPE = "Y2H";
            temp = current_date.getRangeEnd(current_date.train_range - 1);
            if (current_date.getYear() != temp.getYear()) {
                return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "~" + temp.getYear() + "_" + temp.getQ() + "(" + current_date.getYear() + " Q1).csv";
            }
            else {
                return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "(" + current_date.getYear() + " Q1).csv";
            }
            break;
        case 9://Y2Y
            TYPE = "Y2Y";
            return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "(" + current_date.getYear() + " Q1).csv";
            break;
        case 10://M#
            TYPE = "M#";
            return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "(" + current_date.getYear() + " Q1).csv";
            break;
        case 11://Q#
            TYPE = "Q#";
            return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "(" + current_date.getYear() + " Q1).csv";
            break;
        case 12://H#
            TYPE = "H#";
            temp = current_date.getRangeEnd(current_date.slide_number - 1);
            return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "-" + temp.getQ() + "(" + current_date.getYear() + " Q1).csv";
            break;
        }
    }
    else {
        switch (SLIDETYPE) {
        case 0://M2M
            TYPE = "M2M";
            return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "(" + current_date.getRangeEnd(-1 * current_date.train_range).getYear() + " Q1).csv";
            break;
        case 1://Q2M
            TYPE = "Q2M";
            temp = current_date.getRangeEnd(current_date.slide_number - 1);
            if (current_date.getYear() != temp.getYear()) {
                return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "~" + temp.getYear() + "_" + temp.getMon() + "(" + current_date.getRangeEnd(-1 * current_date.train_range).getYear() + " Q1).csv";
            }
            else {
                return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "(" + current_date.getRangeEnd(-1 * current_date.train_range).getYear() + " Q1).csv";
            }
            break;
        case 2://Q2Q
            TYPE = "Q2Q";
            return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "(" + current_date.getRangeEnd(-1 * current_date.train_range).getYear() + " Q1).csv";
            break;
        case 3://H2M
            TYPE = "H2M";
            temp = current_date.getRangeEnd(current_date.slide_number - 1);
            if (current_date.getYear() != temp.getYear()) {
                return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "~" + temp.getYear() + "_" + temp.getMon() + "(" + current_date.getRangeEnd(-1 * current_date.train_range).getYear() + " Q1).csv";
            }
            else {
                return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "(" + current_date.getRangeEnd(-1 * current_date.train_range).getYear() + " Q1).csv";
            }
            break;
        case 4://H2Q
            TYPE = "H2Q";
            temp = current_date.getRangeEnd(current_date.slide_number - 1);
            if (current_date.getYear() != temp.getYear()) {
                return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "~" + temp.getYear() + "_" + temp.getQ() + "(" + current_date.getRangeEnd(-1 * current_date.train_range).getYear() + " Q1).csv";
            }
            else {
                return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "(" + current_date.getRangeEnd(-1 * current_date.train_range).getYear() + " Q1).csv";
            }
            break;
        case 5://H2H
            TYPE = "H2H";
            temp = current_date.getRangeEnd(current_date.slide_number - 1);
            return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "-" + temp.getQ() + "(" + current_date.getRangeEnd(-1 * current_date.train_range).getYear() + " Q1).csv";
            break;
        case 6://Y2M
            TYPE = "Y2M";
            temp = current_date.getRangeEnd(current_date.slide_number - 1);
            if (current_date.getYear() != temp.getYear()) {
                return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "~" + temp.getYear() + "_" + temp.getMon() + "(" + current_date.getRangeEnd(-1 * current_date.train_range).getYear() + " Q1).csv";
            }
            else {
                return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "(" + current_date.getRangeEnd(-1 * current_date.train_range).getYear() + " Q1).csv";
            }
            break;
        case 7://Y2Q
            TYPE = "Y2Q";
            temp = current_date.getRangeEnd(current_date.slide_number - 1);
            if (current_date.getYear() != temp.getYear()) {
                return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "~" + temp.getYear() + "_" + temp.getQ() + "(" + current_date.getRangeEnd(-1 * current_date.train_range).getYear() + " Q1).csv";
            }
            else {
                return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "(" + current_date.getRangeEnd(-1 * current_date.train_range).getYear() + " Q1).csv";
            }
            break;
        case 8://Y2H
            TYPE = "Y2H";
            temp = current_date.getRangeEnd(current_date.slide_number - 1);
            if (current_date.getYear() != temp.getYear()) {
                return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "~" + temp.getYear() + "_" + temp.getQ() + "(" + current_date.getRangeEnd(-1 * current_date.train_range).getYear() + " Q1).csv";
            }
            else {
                return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "-" + temp.getQ() + "(" + current_date.getRangeEnd(-1 * current_date.train_range).getYear() + " Q1).csv";
            }
            break;
        case 9://Y2Y
            TYPE = "Y2Y";
            return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "(" + current_date.getRangeEnd(-1 * current_date.train_range).getYear() + " Q1).csv";
            break;
        case 10://M#
            TYPE = "M#";
            return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "(" + current_date.getRangeEnd(-1 * current_date.train_range).getYear() + " Q1).csv";
            break;
        case 11://Q#
            TYPE = "Q#";
            return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "(" + current_date.getRangeEnd(-1 * current_date.train_range).getYear() + " Q1).csv";
            break;
        case 12://H#
            TYPE = "H#";
            temp = current_date.getRangeEnd(5);
            return DATA_FILE_DIR + "/" + TYPE + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "-" + temp.getQ() + "(" + current_date.getRangeEnd(-1 * current_date.train_range).getYear() + " Q1).csv";
            break;
        }
    }
    return "";
}

string getOutputFilePath(Date current_date, string mode, string file_dir, string type) {
    return file_dir + "/" + type + "/" + mode + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + ".csv";
}

string getOutputFilePath(Date current_date, string mode, string file_dir, string type, string company_name) {
    return file_dir + "/" + type + "/" + mode + "/" + company_name + "/" + company_name + "_" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + ".csv";
}


void initial(double* b, int size) {
    for (int j = 0; j < size; j++) {
        b[j] = 0.5;
    }
}

void initPortfolio(Portfolio* p, int portfolio_number, int size, int day_number, Stock* stock_list, double funds) {
    for (int j = 0; j < portfolio_number; j++) {
        p[j].init(size, day_number, funds, stock_list);
    }
}

void initPortfolio(Portfolio* p, int portfolio_number) {
    for (int j = 0; j < portfolio_number; j++) {
        p[j].init();
    }
}

void genPortfolio(Portfolio* portfolio_list, Stock* stock_list, int portfolio_number, double* beta_, int n, int i) {


    for (int j = 0; j < portfolio_number; j++) {

        portfolio_list[j].exp = n + 1;
        portfolio_list[j].gen = i + 1;
        portfolio_list[j].stock_number = 0;
        for (int k = 0; k < portfolio_list[j].size; k++) {
            double r = (double)rand() / (double)RAND_MAX;
            if (r > beta_[k]) {
                portfolio_list[j].data[k] = 0;
            }
            else {
                portfolio_list[j].data[k] = 1;
            }
        }

        for (int k = 0; k < portfolio_list[j].size; k++) {
            if (portfolio_list[j].data[k] == 1) {
                portfolio_list[j].stock_id_list[portfolio_list[j].stock_number] = k;
                portfolio_list[j].stock_number++;
            }
        }
    }
}

void genPortfolio(Portfolio* portfolio_list, Stock* stock_list, int portfolio_number, int company_id) {
    for (int j = 0; j < portfolio_number; j++) {
        portfolio_list[j].stock_number = 0;
        for (int k = 0; k < portfolio_list[j].size; k++) {
            if (k == company_id) {
                portfolio_list[j].data[k] = 1;
            }
            else {
                portfolio_list[j].data[k] = 0;
            }
        }

        for (int k = 0; k < portfolio_list[j].size; k++) {
            if (portfolio_list[j].data[k] == 1) {
                portfolio_list[j].stock_id_list[portfolio_list[j].stock_number] = k;
                portfolio_list[j].stock_number++;
            }
        }
    }
}

void genTestPortfolio(Portfolio* portfolio_list, Stock* stock_list, int portfolio_number, string* myTrainData, int myTrainData_size) {
    for (int j = 0; j < portfolio_number; j++) {
        for (int k = 0; k < portfolio_list[j].size; k++) {
            if (portfolio_list[j].stock_number < myTrainData_size && portfolio_list[j].constituent_stocks[k].company_name == myTrainData[portfolio_list[j].stock_number]) {
                portfolio_list[j].data[k] = 1;
                portfolio_list[j].stock_id_list[portfolio_list[j].stock_number] = k;
                portfolio_list[j].stock_number++;
            }
            else {
                portfolio_list[j].data[k] = 0;
            }
        }
    }
}

void genTestPortfolio(Portfolio* portfolio_list, Stock* stock_list, int portfolio_number, Portfolio& temp_portfolio) {
    for (int j = 0; j < portfolio_number; j++) {
        for (int k = 0; k < portfolio_list[j].size; k++) {
            if (k == temp_portfolio.stock_id_list[portfolio_list[j].stock_number] && portfolio_list[j].stock_number < temp_portfolio.stock_number) {
                portfolio_list[j].data[k] = 1;
                portfolio_list[j].stock_id_list[portfolio_list[j].stock_number] = k;
                portfolio_list[j].stock_number++;
            }
            else {
                portfolio_list[j].data[k] = 0;
            }

        }
    }
}


void capitalLevel(Portfolio* portfolio_list, int portfolio_number) {
    for (int j = 0; j < portfolio_number; j++) {
        for (int k = 0; k < portfolio_list[j].stock_number; k++) {
            portfolio_list[j].investment_number[k] = portfolio_list[j].getDMoney() / portfolio_list[j].constituent_stocks[portfolio_list[j].stock_id_list[k]].price_list[0];
            portfolio_list[j].remain_fund[k] = portfolio_list[j].getDMoney() - (portfolio_list[j].investment_number[k] * portfolio_list[j].constituent_stocks[portfolio_list[j].stock_id_list[k]].price_list[0]);
        }
        //        portfolio_list[j].total_money[0] = funds;
        for (int k = 0; k < portfolio_list[j].day_number; k++) {
            portfolio_list[j].total_money[k] = portfolio_list[j].getRemainMoney();
            for (int h = 0; h < portfolio_list[j].stock_number; h++) {
                portfolio_list[j].total_money[k] += portfolio_list[j].investment_number[h] * portfolio_list[j].constituent_stocks[portfolio_list[j].stock_id_list[h]].price_list[k + 1];
            }
            if (portfolio_list[j].total_money[k] > portfolio_list[j].capital_highest_point) {
                portfolio_list[j].capital_highest_point = portfolio_list[j].total_money[k];
            }
            double DD = (portfolio_list[j].capital_highest_point - portfolio_list[j].total_money[k]) / portfolio_list[j].capital_highest_point;
            if (DD > portfolio_list[j].MDD) {
                portfolio_list[j].MDD = DD;
            }
        }
    }
}

void capitalLevel(Portfolio* portfolio_list, int portfolio_number, Portfolio& result) {
    for (int j = 0; j < portfolio_number; j++) {
        for (int k = 0; k < portfolio_list[0].stock_number; k++) {
            portfolio_list[0].investment_number[k] = result.investment_number[k];
            portfolio_list[0].remain_fund[k] = result.remain_fund[k];
        }
        //        portfolio_list[j].total_money[0] = funds;
        for (int k = 0; k < portfolio_list[j].day_number; k++) {
            portfolio_list[j].total_money[k] = portfolio_list[j].getRemainMoney();
            for (int h = 0; h < portfolio_list[j].stock_number; h++) {
                portfolio_list[j].total_money[k] += portfolio_list[j].investment_number[h] * portfolio_list[j].constituent_stocks[portfolio_list[j].stock_id_list[h]].price_list[k];
            }
            if (portfolio_list[j].total_money[k] > portfolio_list[j].capital_highest_point) {
                portfolio_list[j].capital_highest_point = portfolio_list[j].total_money[k];
            }
            double DD = (portfolio_list[j].capital_highest_point - portfolio_list[j].total_money[k]) / portfolio_list[j].capital_highest_point;
            if (DD > portfolio_list[j].MDD) {
                portfolio_list[j].MDD = DD;
            }
        }
    }
}

void countTrend(Portfolio* portfolio_list, int porfolio_number, double funds) {

    for (int j = 0; j < porfolio_number; j++) {
        double sum = 0;
        if (TRENDLINETYPE == 0) {
            //portfolio_list[j].countQuadraticYLine();
            double x = 0;
            double y = 0;
            for (int k = 0; k < portfolio_list[j].day_number; k++) {
                x += (k + 1) * (portfolio_list[j].total_money[k] - funds);
                y += (k + 1) * (k + 1);
            }
            if (portfolio_list[j].stock_number != 0) {
                portfolio_list[j].m = x / y;
            }
            for (int k = 0; k < portfolio_list[j].day_number; k++) {
                double Y;
                Y = portfolio_list[j].getNormalY(k + 1);
                sum += (portfolio_list[j].total_money[k] - Y) * (portfolio_list[j].total_money[k] - Y);
            }
        }
        else if (TRENDLINETYPE == 1) {
            portfolio_list[j].countQuadraticYLine();
            for (int k = 0; k < portfolio_list[j].day_number; k++) {
                double Y;
                Y = portfolio_list[j].getQuadraticY(k + 1);
                sum += (portfolio_list[j].total_money[k] - Y) * (portfolio_list[j].total_money[k] - Y);
            }
            portfolio_list[j].m = (portfolio_list[j].getQuadraticY(portfolio_list[j].day_number) - portfolio_list[j].getQuadraticY(1)) / (portfolio_list[j].day_number - 1);
        }

        portfolio_list[j].daily_risk = sqrt(sum / (portfolio_list[j].day_number));

        if (portfolio_list[j].m < 0) {
            portfolio_list[j].trend = portfolio_list[j].m * portfolio_list[j].daily_risk;
        }
        else {
            portfolio_list[j].trend = portfolio_list[j].m / portfolio_list[j].daily_risk;
        }
    }
}

void recordGAnswer(Portfolio* portfolio_list, Portfolio& gBest, Portfolio& gWorst, Portfolio& pBest, Portfolio& pWorst) {
    pBest.copyP(portfolio_list[0]);
    pWorst.copyP(portfolio_list[PARTICLENUMBER - 1]);
    for (int j = 0; j < PARTICLENUMBER; j++) {
        if (pBest.trend < portfolio_list[j].trend) {
            pBest.copyP(portfolio_list[j]);
        }
        if (pWorst.trend > portfolio_list[j].trend) {
            pWorst.copyP(portfolio_list[j]);
        }
    }

    if (gBest.trend < pBest.trend) {
        gBest.copyP(pBest);
    }

    if (gWorst.trend > pWorst.trend) {
        gWorst.copyP(pWorst);
    }
}

void adjBeta(Portfolio& best, Portfolio& worst, double* beta_) {
    for (int j = 0; j < best.size; j++) {
        if (QTSTYPE == 3) {
            DELTA = LOWER + (UPPER - LOWER) * (1 - 2 * abs(beta_[j] - 0.5));
            if (best.data[j] > worst.data[j]) {
                if (beta_[j] < 0.5) {
                    beta_[j] = 1 - beta_[j];
                }
                beta_[j] += DELTA;
            }
            else if (best.data[j] < worst.data[j]) {
                if (beta_[j] > 0.5) {
                    beta_[j] = 1 - beta_[j];
                }
                beta_[j] -= DELTA;
            }
        }
        else if (QTSTYPE == 2) {
            if (best.data[j] > worst.data[j]) {
                if (beta_[j] < 0.5) {
                    beta_[j] = 1 - beta_[j];
                }
                beta_[j] += DELTA;
            }
            else if (best.data[j] < worst.data[j]) {
                if (beta_[j] > 0.5) {
                    beta_[j] = 1 - beta_[j];
                }
                beta_[j] -= DELTA;
            }
        }
        else if (QTSTYPE == 1) {
            if (best.data[j] > worst.data[j]) {
                beta_[j] += DELTA;
            }
            else if (best.data[j] < worst.data[j]) {
                beta_[j] -= DELTA;
            }
        }
        else {
            if (best.data[j] > worst.data[j]) {
                beta_[j] += DELTA;
            }
            else if (best.data[j] < worst.data[j]) {
                beta_[j] -= DELTA;
            }
        }
    }
}

void recordExpAnswer(Portfolio& expBest, Portfolio& gBest) {
    if (expBest.trend < gBest.trend) {
        expBest.copyP(gBest);
        expBest.answer_counter = 1;
    }
    else if (expBest.trend == gBest.trend) {
        expBest.answer_counter++;
    }
}

void countDPF(Portfolio &portfolio) {

    double last_money = portfolio.funds;
    double pos_trend = 0;
    double neg_trend = 0;

    for (int j = 0; j < portfolio.day_number; j++) {
        if ((portfolio.total_money[j] - last_money) >= 0) {
            pos_trend += portfolio.total_money[j] - last_money;
        }
        else {
            neg_trend += -1 * (portfolio.total_money[j] - last_money);
        }

        last_money = portfolio.total_money[j];
    }

    if (neg_trend == 0) {
        portfolio.DPF = -1;
    }
    else {
        portfolio.DPF = pos_trend / neg_trend;
    }
}

void countMR(Portfolio& portfolio) {
    double mr_m = ((portfolio.funds + portfolio.getProfit()) - portfolio.total_money[0]) / (portfolio.day_number - 1);
    double b = portfolio.total_money[0];
    double sum = 0;
    for (int j = 0; j < portfolio.day_number; j++) {
        double Y;
        Y = mr_m * (j + 1) + b;
        sum += (portfolio.total_money[j] - Y) * (portfolio.total_money[j] - Y);
    }
    double mr_daily_risk = sqrt(sum / (portfolio.day_number));
    portfolio.mood_ratio = mr_m / mr_daily_risk;
}

void releaseArray(string** a, int length) {
    for (int j = 0; j < length; j++) {
        delete[] a[j];
    }
    delete[] a;
}

void releaseVector(vector<vector<string>> v) {
    for (int j = 0; j < v.size(); j++) {
        v[j].clear();
    }
    v.clear();
}


void recordTotalTestResult(vector<double>& total_fs, vector<string>& total_date, string TYPE) {
    Portfolio portfolio(1, total_fs.size(), FUNDS);
    portfolio.stock_number = 1;
    for (int j = 0; j < total_fs.size(); j++) {
        portfolio.total_money[j] = total_fs[j];
        portfolio.date_list[j] = total_date[j];
    }
    countTrend(&portfolio, 1, FUNDS);
    countDPF(portfolio);
    countMR(portfolio);
    ofstream outfile_total_result;
    string file_name = FILE_DIR + "/" + TYPE + "/total_test_result.csv";
    outfile_total_result.open(file_name, ios::out);
    outfile_total_result << setprecision(15);

    outfile_total_result << "Test Date," << total_date[0] << "-" << total_date[total_date.size() - 1] << endl;
    outfile_total_result << "Total days," << portfolio.day_number << endl;
    outfile_total_result << "Iteration," << ITERNUMBER << endl;
    outfile_total_result << "Element number," << PARTICLENUMBER << endl;
    outfile_total_result << "Delta," << DELTA << endl;
    outfile_total_result << "Exp times," << EXPNUMBER << endl << endl;

    outfile_total_result << "Init funds," << portfolio.funds << endl;
    outfile_total_result << "Final funds," << portfolio.total_money[portfolio.day_number - 1] << endl;
    outfile_total_result << "Real award," << portfolio.getProfit() << endl << endl;

    outfile_total_result << "m," << portfolio.m << endl;
    outfile_total_result << "Daily_risk," << portfolio.daily_risk << endl;
    outfile_total_result << "Trend," << portfolio.trend << endl << endl;

    if (TRENDLINETYPE == 0) {
        portfolio.countQuadraticYLine();
        double sum = 0;
        for (int k = 0; k < portfolio.day_number; k++) {
            double Y;
            Y = portfolio.getQuadraticY(k + 1);
            sum += (portfolio.total_money[k] - Y) * (portfolio.total_money[k] - Y);
        }
        double c = (portfolio.getQuadraticY(portfolio.day_number) - portfolio.getQuadraticY(1)) / (portfolio.day_number - 1);
        double d = sqrt(sum / (portfolio.day_number));

        outfile_total_result << "Quadratic trend line," << portfolio.a << "x^2 + " << portfolio.b << "x + " << FUNDS << endl << endl;
        outfile_total_result << "Quadratic m," << c << endl;
        outfile_total_result << "Quadratic daily risk," << d << endl;
        if (c < 0) {
            outfile_total_result << "Quadratic trend," << c * d << endl << endl;
        }
        else {
            outfile_total_result << "Quadratic trend," << c / d << endl << endl;
        }
    }
    else {
        outfile_total_result << "Quadratic trend line," << portfolio.a << "x^2 + " << portfolio.b << "x + " << FUNDS << endl << endl;
        double x = 0;
        double y = 1;
        double sum = 0;
        for (int k = 0; k < portfolio.day_number - 1; k++) {
            x += (k + 2) * (portfolio.total_money[k + 1] - portfolio.funds);
            y += (k + 2) * (k + 2);
        }

        double c = x / y;
        for (int k = 0; k < portfolio.day_number; k++) {
            double Y;
            Y = c * (k + 1) + portfolio.funds;
            sum += (portfolio.total_money[k] - Y) * (portfolio.total_money[k] - Y);
        }
        double d = sqrt(sum / (portfolio.day_number));

        outfile_total_result << "Linear m," << c << endl;
        outfile_total_result << "Linear daily risk," << d << endl;
        if (c < 0) {
            outfile_total_result << "Linear trend," << c * d << endl << endl;
        }
        else {
            outfile_total_result << "Linear trend," << c / d << endl << endl;
        }
    }

    double DD = 0;
    double MDD = 0;
    double highest_point = 0;
    for (int j = 0; j < portfolio.day_number; j++) {
        if (portfolio.total_money[j] > highest_point) {
            highest_point = portfolio.total_money[j];
        }
        DD = (highest_point - portfolio.total_money[j]) / highest_point;
        if (DD > portfolio.MDD) {
            portfolio.MDD = DD;
        }
    }

    outfile_total_result << "MDD," << portfolio.MDD << endl;
    outfile_total_result << "DPF," << portfolio.DPF << endl;
    outfile_total_result << "Mood ratio," << portfolio.mood_ratio << endl;

    outfile_total_result << "Date,FS" << endl;
    for (int j = 0; j < portfolio.day_number; j++) {
        outfile_total_result << portfolio.date_list[j] << "," << portfolio.total_money[j] << endl;
    }
    outfile_total_result.close();
}



void startTrain(Portfolio& result, Stock* stock_list, int size, int range_day_number) {
    double* beta_ = new double[size];
    Portfolio expBest(size, range_day_number, FUNDS, stock_list);
    Portfolio gBest(size, range_day_number, FUNDS, stock_list);
    Portfolio gWorst(size, range_day_number, FUNDS, stock_list);
    Portfolio pBest(size, range_day_number, FUNDS, stock_list);
    Portfolio pWorst(size, range_day_number, FUNDS, stock_list);
    Portfolio* portfolio_list = new Portfolio[PARTICLENUMBER];
    initPortfolio(portfolio_list, PARTICLENUMBER, size, range_day_number, stock_list, FUNDS);

    for (int n = 0; n < EXPNUMBER; n++) {
        cout << "___" << n << "___" << endl;
        gBest.init();
        gWorst.init();
        gBest.trend = 0;
        gWorst.trend = DBL_MAX;
        initial(beta_, size);
        for (int i = 0; i < ITERNUMBER; i++) {
            pBest.init();
            pWorst.init();
            initPortfolio(portfolio_list, PARTICLENUMBER);
            genPortfolio(portfolio_list, stock_list, PARTICLENUMBER, beta_, n, i);
            capitalLevel(portfolio_list, PARTICLENUMBER);
            countTrend(portfolio_list, PARTICLENUMBER, FUNDS);
            recordGAnswer(portfolio_list, gBest, gWorst, pBest, pWorst);
            if (QTSTYPE != 0) {
                adjBeta(gBest, pWorst, beta_);
            }
            else {
                adjBeta(pBest, pWorst, beta_);
            }
        }
        recordExpAnswer(expBest, gBest);
    }

    //expBest.print();
    delete[] portfolio_list;
    delete[] beta_;
    result.copyP(expBest);

}



void startTest(Portfolio& result, Stock* stock_list, string* myTrainData, int myTrainData_size, int size, int range_day_number, double test_fund) {

    Portfolio* portfolio_list = new Portfolio[1];
    initPortfolio(portfolio_list, 1, size, range_day_number, stock_list, test_fund);
    genTestPortfolio(portfolio_list, stock_list, 1, myTrainData, myTrainData_size);
    capitalLevel(portfolio_list, 1);
    countTrend(portfolio_list, 1, FUNDS);
    result.copyP(portfolio_list[0]);
    delete[] portfolio_list;
}

void startSin(Portfolio& result, Stock* stock_list, int company_id, int size, int range_day_number) {

    Portfolio* portfolio_list = new Portfolio[1];
    initPortfolio(portfolio_list, 1, size, range_day_number, stock_list, FUNDS);
    genPortfolio(portfolio_list, stock_list, 1, company_id);
    capitalLevel(portfolio_list, 1);
    countTrend(portfolio_list, 1, FUNDS);
    result.copyP(portfolio_list[0]);
    delete[] portfolio_list;
}

int main(int argc, const char* argv[]) {
    Date current_date;
    Date finish_date;
    string TYPE;
    string** data;
    string* data_copy;
    string temp;
    vector<vector<string>> data_vector;
    int size;
    int day_number;
    double START, END;
    Stock* stock_list;

    for (int s = 1; s < 2; s++) {

        srand(114);
        cout << setprecision(15);

        START = clock();
        preSet(MODE, current_date, finish_date, s, TYPE);
        createDir(FILE_DIR, TYPE, MODE);
        cout << TYPE << endl;

        readData("2011-2020.csv", data_vector, size, day_number);
        data = vectorToArray(data_vector);
        data_copy = new string[day_number];
        copyData(data_copy, data, day_number);

        temp = FILE_DIR + "/" + TYPE + "/" + MODE + "_detail.csv";
        ofstream outfile_detail;
        outfile_detail.open(temp, ios::out);
        outfile_detail << setprecision(15);
        outfile_detail << "Date,Real award,m,Daily risk,Trend,MDD,DPF,Mood ratio,Stock number,Stock" << endl;
        temp = FILE_DIR + "/" + TYPE + "/" + "total_data.csv";
        ofstream outfile_total_data;

        double test_fund = FUNDS;
        vector<double> total_fs;
        vector<string> total_date;

        if (MODE == "test") {
            outfile_total_data.open(temp, ios::out);
            outfile_total_data << setprecision(15);
            outfile_total_data << "train date,test date,test day number,MDD,profit,DPF,LN_trend,QD_trend,Mood ratio,Stop flag,Stock number,Stock" << endl;
        }

        do {
            int range_day_number;
            string start_date;
            string end_date;
            int start_index;
            int end_index;

            setWindow(MODE, start_date, end_date, start_index, end_index, current_date, finish_date, data_copy, data, day_number, range_day_number);

            cout << "______" << TYPE << " : " << current_date.getDate() << " - " << current_date.getRangeEnd(current_date.train_range - 1).getDate() << "______" << endl;
            stock_list = new Stock[size];
            createStock(stock_list, size, range_day_number, data, start_index, end_index);
            Portfolio result(size, range_day_number, FUNDS, stock_list);
            
            if (MODE == "train") {
                result.date = current_date.getDate() + " - " + current_date.getRangeEnd(current_date.train_range - 1).getDate();
                startTrain(result, stock_list, size, range_day_number);
                countDPF(result);
                countMR(result);
                if (result.trend != 0) {
                    outputFile(result, getOutputFilePath(current_date, MODE, FILE_DIR, TYPE));
                }
                recordDetail(outfile_detail, result);
            }
            else if (MODE == "test") {
                result.date = current_date.getDate() + " - " + current_date.getRangeEnd(current_date.slide_number - 1).getDate();
                vector<string> myTrainData_vector;
                int myTrainData_size = 0;
                bool flag = readSpeData(getOutputFilePath(current_date.getRangeEnd(-1 * current_date.train_range), "train", FILE_DIR, TYPE), myTrainData_vector, myTrainData_size, "Stock#");
                if (flag) {
                    string* myTrainData = vectorToArray(myTrainData_vector);
                    startTest(result, stock_list, myTrainData, myTrainData_size, size, range_day_number, test_fund);
                    countDPF(result);
                    countMR(result);
                    test_fund = result.funds + result.getProfit();
                    myTrainData_vector.clear();
                    delete[] myTrainData;
                    outputFile(result, getOutputFilePath(current_date, MODE, FILE_DIR, TYPE));
                    recordDetail(outfile_detail, result);
                    recordTotalResult(outfile_total_data, result, current_date.getRangeEnd(-1 * current_date.train_range));
                    for (int j = 0; j < result.day_number; j++) {
                        total_fs.push_back(result.total_money[j]);
                        total_date.push_back(result.date_list[j]);
                    }
                }
                else {
                    for (int j = 0; j < result.day_number; j++) {
                        total_fs.push_back(test_fund);
                        total_date.push_back(result.date_list[j]);
                    }
                }
                
            }
            else if (MODE == "exhaustive") {
                //                startExhaustive(result, company_list[c], companyData, range_day_number);
            }
            else if (MODE == "B&H") {
                //                startBH(result, company_list[c], companyData, range_day_number);
            }
            else if (MODE == "single" && current_date.getDate() == "201906") {
                for (int j = 0; j < size; j++) {
                    createDir(FILE_DIR, TYPE, MODE, stock_list[j].company_name);
                    startSin(result, stock_list, j, size, day_number);
                    outputFile(result, getOutputFilePath(current_date, MODE, FILE_DIR, TYPE, stock_list[j].company_name));
                }
            }
            delete[] stock_list;
            current_date.slide();
        } while (finish_date >= current_date);
        outfile_detail.close();
        outfile_total_data.close();
        releaseArray(data, day_number + 1);
        releaseVector(data_vector);
        delete[] data_copy;
        END = clock();
        temp = FILE_DIR + "/" + TYPE + "/" + "time_" + MODE + ".txt";
        recordCPUTime(START, END, temp);
        if (MODE == "test") {
            recordTotalTestResult(total_fs, total_date, TYPE);
        }
        total_fs.clear();
        total_date.clear();

    }

    return 0;
}
