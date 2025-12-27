#include "../include/generate_data_mf.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cctype>
#include <stdexcept>

using namespace std;

// Товар в заказе
struct Item {
    string sku;        // Артикул товара
    int quantity;      // Количество
    double price;      // Цена за штуку
};

// Один заказ (продажа)
struct Order {
    string id;              // Номер заказа
    string date_time;       // Дата и время
    vector<Item> items;     // Список товаров
};

// Убрать пробелы в начале строки
void skip_spaces(const string& text, int& position) {
    while (position < text.length() && (text[position] == ' ' ||
                                        text[position] == '\n' || text[position] == '\t' || text[position] == '\r')) {
        position++;
    }
}

// Прочитать строку из JSON (в кавычках)
string read_json_string(const string& text, int& position) {
    skip_spaces(text, position);

    // Пропускаем открывающую кавычку
    if (text[position] != '"') {
        cout << "Ошибка: ожидалась кавычка на позиции " << position << endl;
        return "";
    }
    position++;

    // Читаем символы до закрывающей кавычки
    string result = "";
    while (position < text.length() && text[position] != '"') {
        if (text[position] == '\\') {  // Обработка спецсимволов
            position++;
            if (text[position] == 'n') result += '\n';
            else if (text[position] == 't') result += '\t';
            else result += text[position];
        }
        else {
            result += text[position];
        }
        position++;
    }

    // Пропускаем закрывающую кавычку
    position++;
    return result;
}

// Прочитать число из JSON
double read_json_number(const string& text, int& position) {
    skip_spaces(text, position);

    string number_string = "";

    // Минус для отрицательных чисел
    if (text[position] == '-') {
        number_string += text[position];
        position++;
    }

    // Читаем цифры
    while (position < text.length() && (isdigit(text[position]) || text[position] == '.')) {
        number_string += text[position];
        position++;
    }

    // Преобразуем строку в число
    return stod(number_string);
}

// Прочитать один товар из JSON
Item read_json_item(const string& text, int& position) {
    Item item;

    skip_spaces(text, position);
    position++; // Пропускаем {

    // Читаем поля товара
    while (text[position] != '}') {
        skip_spaces(text, position);

        if (text[position] == ',') {
            position++;
            continue;
        }

        if (text[position] == '}') break;

        // Читаем имя поля
        string field_name = read_json_string(text, position);

        skip_spaces(text, position);
        position++; // Пропускаем :

        // Читаем значение поля
        if (field_name == "sku") {
            item.sku = read_json_string(text, position);
        }
        else if (field_name == "qty") {
            item.quantity = (int)read_json_number(text, position);
        }
        else if (field_name == "price") {
            item.price = read_json_number(text, position);
        }
    }

    position++; // Пропускаем }
    return item;
}

// Прочитать один заказ из JSON
Order read_json_order(const string& text, int& position) {
    Order order;

    skip_spaces(text, position);
    position++; // Пропускаем {

    // Читаем поля заказа
    while (text[position] != '}') {
        skip_spaces(text, position);

        if (text[position] == ',') {
            position++;
            continue;
        }

        if (text[position] == '}') break;

        // Читаем имя поля
        string field_name = read_json_string(text, position);

        skip_spaces(text, position);
        position++; // Пропускаем :

        // Читаем значение поля
        if (field_name == "id") {
            order.id = read_json_string(text, position);
        }
        else if (field_name == "ts") {
            order.date_time = read_json_string(text, position);
        }
        else if (field_name == "items") {
            skip_spaces(text, position);
            position++; // Пропускаем [

            // Читаем все товары
            while (text[position] != ']') {
                skip_spaces(text, position);

                if (text[position] == ',') {
                    position++;
                    continue;
                }

                if (text[position] == ']') break;

                Item item = read_json_item(text, position);
                order.items.push_back(item);
            }

            position++; // Пропускаем ]
        }
    }

    position++; // Пропускаем }
    return order;
}

// Прочитать все заказы из JSON
vector<Order> read_json(const string& text) {
    vector<Order> orders;
    int position = 0;

    skip_spaces(text, position);
    position++; // Пропускаем [

    // Читаем все заказы
    while (position < text.length() && text[position] != ']') {
        skip_spaces(text, position);

        if (text[position] == ',') {
            position++;
            continue;
        }

        if (text[position] == ']') break;

        Order order = read_json_order(text, position);
        orders.push_back(order);
    }

    return orders;
}

// ========== НОВЫЕ ФУНКЦИИ ДЛЯ РАБОТЫ С ДИРЕКТОРИЯМИ ==========

// Проверить, является ли путь директорией
bool is_directory(const string& path) {
    struct stat statbuf;
    if (stat(path.c_str(), &statbuf) != 0) {
        return false;
    }
    return S_ISDIR(statbuf.st_mode);
}

// Показать доступные поддиректории
void show_available_directories(const string& base_path) {
    cout << endl;
    cout << "Найдены следующие директории в " << base_path << ":" << endl;
    cout << "----------------------------------------------------------------------" << endl;

    DIR* dir = opendir(base_path.c_str());
    if (dir == nullptr) {
        cerr << "Ошибка: не могу открыть директорию " << base_path << endl;
        return;
    }

    vector<string> subdirs;
    struct dirent* entry;

    while ((entry = readdir(dir)) != nullptr) {
        string name = entry->d_name;

        // Пропускаем . и ..
        if (name == "." || name == "..") continue;

        string full_path = base_path + "/" + name;

        // Проверяем, что это директория
        if (is_directory(full_path)) {
            subdirs.push_back(name);
        }
    }
    closedir(dir);

    // Сортируем
    sort(subdirs.begin(), subdirs.end());

    if (subdirs.empty()) {
        cout << "  (нет поддиректорий)" << endl;
    } else {
        for (size_t i = 0; i < subdirs.size(); i++) {
            string subdir_path = base_path + "/" + subdirs[i];

            // Подсчитываем JSON файлы
            DIR* sub = opendir(subdir_path.c_str());
            int json_count = 0;
            if (sub != nullptr) {
                struct dirent* sub_entry;
                while ((sub_entry = readdir(sub)) != nullptr) {
                    string fname = sub_entry->d_name;
                    if (fname.find(".json") != string::npos) {
                        json_count++;
                    }
                }
                closedir(sub);
            }

            cout << "  " << (i+1) << ". " << subdirs[i];
            if (json_count > 0) {
                cout << " (" << json_count << " JSON файлов)";
            }
            cout << endl;
        }
    }

    cout << "----------------------------------------------------------------------" << endl;
    cout << endl;
    cout << "Используйте:" << endl;
    cout << "  ./sales --input " << base_path << "<имя_директории>" << endl;
    cout << endl;
}


// Прочитать один файл с заказами
vector<Order> read_single_file(const string& filepath) {
    ifstream file(filepath);
    if (!file.is_open()) {
        cerr << "Предупреждение: не могу открыть файл " << filepath << endl;
        return vector<Order>();
    }

    string json_text = "";
    string line;
    while (getline(file, line)) {
        json_text += line;
    }
    file.close();

    return read_json(json_text);
}

// Прочитать все JSON файлы из директории
vector<Order> read_directory(const string& dir_path, bool show_progress = true) {
    vector<Order> all_orders;

    DIR* dir = opendir(dir_path.c_str());
    if (dir == nullptr) {
        cerr << "Ошибка: не могу открыть директорию " << dir_path << endl;
        return all_orders;
    }

    // Собираем имена всех JSON файлов
    vector<string> filenames;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        string filename = entry->d_name;

        // Пропускаем . и ..
        if (filename == "." || filename == "..") continue;

        // Берём только .json файлы
        if (filename.find(".json") == string::npos) continue;

        filenames.push_back(filename);
    }
    closedir(dir);

    // Сортируем имена файлов для последовательной обработки
    sort(filenames.begin(), filenames.end());

    // Если нет JSON файлов, показываем поддиректории
    if (filenames.empty()) {
        cout << endl;
        cout << "В директории " << dir_path << " нет JSON файлов." << endl;
        show_available_directories(dir_path);
        return all_orders;
    }

    int total = filenames.size();
    int processed = 0;

    if (show_progress) {
        cout << "Найдено JSON файлов: " << total << endl;
    }

    // Читаем каждый файл
    for (const string& filename : filenames) {
        string filepath = dir_path + "/" + filename;

        vector<Order> orders = read_single_file(filepath);

        // Добавляем все заказы из файла
        for (const Order& order : orders) {
            if (!order.id.empty()) {
                all_orders.push_back(order);
            }
        }

        processed++;

        // Показываем прогресс для больших директорий
        if (show_progress && total >= 100 && processed % (total / 10) == 0) {
            cout << "  Прочитано файлов: " << processed << "/" << total
                 << " (" << (processed * 100 / total) << "%)" << endl;
        }
    }

    return all_orders;
}

// ========== КОНЕЦ НОВЫХ ФУНКЦИЙ ==========

// Проверить все заказы на правильность
bool check_orders(const vector<Order>& orders) {
    bool all_ok = true;
    int error_count = 0;

    // Проверяем каждый заказ
    for (int i = 0; i < orders.size(); i++) {
        const Order& order = orders[i];

        // Проверка: ID не пустой
        if (order.id.empty()) {
            cout << "  Ошибка в заказе #" << i << ": пустой ID" << endl;
            all_ok = false;
            error_count++;
        }

        // Проверка: дата правильного формата
        if (order.date_time.length() < 10) {
            cout << "  Ошибка в заказе #" << i << " (ID: " << order.id
                 << "): неправильная дата" << endl;
            all_ok = false;
            error_count++;
        }

        // Проверка: есть хотя бы один товар
        if (order.items.empty()) {
            cout << "  Ошибка в заказе #" << i << " (ID: " << order.id
                 << "): нет товаров" << endl;
            all_ok = false;
            error_count++;
        }

        // Проверяем каждый товар в заказе
        for (int j = 0; j < order.items.size(); j++) {
            const Item& item = order.items[j];

            // Проверка: артикул не пустой
            if (item.sku.empty()) {
                cout << "  Ошибка в заказе #" << i << ", товар #" << j
                     << ": пустой артикул" << endl;
                all_ok = false;
                error_count++;
            }

            // Проверка: количество больше 0
            if (item.quantity <= 0) {
                cout << "  Ошибка в заказе #" << i << ", товар #" << j
                     << " (" << item.sku << "): количество должно быть > 0" << endl;
                all_ok = false;
                error_count++;
            }

            // Проверка: цена >= 0
            if (item.price < 0) {
                cout << "  Ошибка в заказе #" << i << ", товар #" << j
                     << " (" << item.sku << "): цена не может быть отрицательной" << endl;
                all_ok = false;
                error_count++;
            }
        }
    }

    if (all_ok) {
        cout << "\nВсе данные правильные!" << endl;
    }
    else {
        cout << "\nНайдено ошибок: " << error_count << endl;
    }

    return all_ok;
}

// Посчитать стоимость одного заказа
double calculate_order_total(const Order& order) {
    double total = 0;
    for (int i = 0; i < order.items.size(); i++) {
        total += order.items[i].quantity * order.items[i].price;
    }
    return total;
}

// Получить дату из строки (YYYY-MM-DD)
string get_date(const string& date_time) {
    if (date_time.length() >= 10) {
        return date_time.substr(0, 10);  // Берем первые 10 символов
    }
    return date_time;
}

// Посчитать выручку по дням
map<string, double> calculate_daily_revenue(const vector<Order>& orders) {
    map<string, double> daily_revenue;  // дата -> выручка
    map<string, int> daily_count;       // дата -> количество заказов

    // Суммируем выручку по каждому дню
    for (int i = 0; i < orders.size(); i++) {
        string date = get_date(orders[i].date_time);
        double order_total = calculate_order_total(orders[i]);

        daily_revenue[date] += order_total;
        daily_count[date]++;
    }

    return daily_revenue;
}

// Посчитать средний чек
double calculate_average_check(const vector<Order>& orders) {
    if (orders.empty()) return 0;

    double total = 0;
    for (int i = 0; i < orders.size(); i++) {
        total += calculate_order_total(orders[i]);
    }

    return total / orders.size();
}

// Сортировка пар в products
bool f(pair<string, double>& a, pair<string, double>& b) {
    return a.second > b.second;
}

// Найти топ товаров по выручке
vector<pair<string, double>> find_top_products(const vector<Order>& orders, int top_count) {
    map<string, double> product_revenue;  // артикул -> выручка

    // Суммируем выручку по каждому товару
    for (int i = 0; i < orders.size(); i++) {
        for (int j = 0; j < orders[i].items.size(); j++) {
            const Item& item = orders[i].items[j];
            product_revenue[item.sku] += item.quantity * item.price;
        }
    }

    // Переносим в вектор для сортировки
    vector<pair<string, double>> products;
    for (map<string, double>::iterator it = product_revenue.begin();
         it != product_revenue.end(); ++it) {
        products.push_back(make_pair(it->first, it->second));
    }

    // Сортируем по выручке (от большего к меньшему)
    /*for (int i = 0; i < products.size(); i++) {
        for (int j = i + 1; j < products.size(); j++) {
            if (products[j].second > products[i].second) {
                swap(products[i], products[j]);
            }
        }
    }*/
    sort(products.begin(), products.end(), f);

    // Оставляем только топ N
    if (products.size() > top_count) {
        products.resize(top_count);
    }

    return products;
}

// Вывести линию
void print_line(int length) {
    for (int i = 0; i < length; i++) {
        cout << "=";
    }
    cout << endl;
}

// Вывести заголовок
void print_header(const string& text) {
    cout << endl;
    print_line(70);
    cout << "  " << text << endl;
    print_line(70);
    cout << endl;
}

// Вывести сумму денег
void print_money(double amount) {
    cout.precision(2);
    cout << fixed << amount << " руб.";
}

// ========== НАЧАЛО ФУНКЦИЙ БЫСТРОГО ТЕСТА ==========
// Поиск следующего номера теста
int get_next_test_index() {
    int n = 1;
    while (true) {
        string path = "data/t" + to_string(n);
        struct stat st;
        if (stat(path.c_str(), &st) != 0) break;
        n++;
    }
    return n;
}

// Создание папки теста
string create_test_directory(int n) {
    string dir = "data/t" + to_string(n);

#ifdef WIN32  // Или _WIN32, или _MSC_VER
    // Windows: mkdir принимает только путь
    if (mkdir("data") != 0 && errno != EEXIST) {
        cerr << "Ошибка создания директории data: " << strerror(errno) << endl;
        return "";  // Возвращаем пустую строку при ошибке
    }
    if (mkdir(dir.c_str()) != 0 && errno != EEXIST) {
        cerr << "Ошибка создания директории " << dir << ": " << strerror(errno) << endl;
        return "";
    }
#else
    // Unix-подобные системы: mkdir с правами доступа
    if (mkdir("data", 0755) != 0 && errno != EEXIST) {
        cerr << "Ошибка создания директории data: " << strerror(errno) << endl;
        return "";
    }
    if (mkdir(dir.c_str(), 0755) != 0 && errno != EEXIST) {
        cerr << "Ошибка создания директории " << dir << ": " << strerror(errno) << endl;
        return "";
    }
#endif

    return dir;  // Возвращаем путь к созданной директории
}

// Benchmark генерации
map<int, long long> benchmark_generation(const string& base_dir) {
    vector<int> sizes = {10, 100, 1000, 10000, 100000, 250000};
    map<int, long long> results;

    for (int n : sizes) {
        string dir = base_dir + "/gen_" + to_string(n);
        auto start = chrono::high_resolution_clock::now();
        generate_separate_files(dir, n, false);
        auto end = chrono::high_resolution_clock::now();
        results[n] = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    }
    return results;
}

// Benchmark обработки
map<int, long long> benchmark_processing(const string& base_dir) {
    vector<int> sizes = {10, 100, 1000, 10000, 100000, 250000};
    map<int, long long> results;

    for (int n : sizes) {
        string dir = base_dir + "/gen_" + to_string(n);

        auto start = chrono::high_resolution_clock::now();
        vector<Order> orders = read_directory(dir, false);
        check_orders(orders);
        calculate_average_check(orders);
        find_top_products(orders, 5);
        auto end = chrono::high_resolution_clock::now();

        results[n] = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    }
    return results;
}

// Генерация .md отчёта
void write_benchmark_report(
        int test_id,
        const map<int, long long>& gen,
        const map<int, long long>& proc
) {
    string filename = "tests/t" + to_string(test_id) + ".md";
    ofstream f(filename);

    f << "# Benchmark Report\n\n";

    f << "### Тесты:";

    f << "1) Генерация n-го кол-ва JSON-файлов.\n\n";
    f << "| Кол-во файлов | Время (ms) |\n";
    f << "|--------------|-------------|\n";
    for (auto& p : gen)
        f << "| " << p.first << " | " << p.second << " |\n";

    f << "\n\n2) Обработка n-го кол-ва JSON-файлов.\n\n";
    f << "| Кол-во файлов | Время (ms) |\n";
    f << "|--------------|-------------|\n";
    for (auto& p : proc)
        f << "| " << p.first << " | " << p.second << " |\n";
}

// быстрый тест
void run_benchmark_tests() {
    int test_id = get_next_test_index();
    string dir = create_test_directory(test_id);

    auto gen_results = benchmark_generation(dir);
    auto proc_results = benchmark_processing(dir);

    write_benchmark_report(test_id, gen_results, proc_results);

    cout << "Benchmark завершён: tests/t" << test_id << endl;
}
// ========== КОНЕЦ ФУНКЦИЙ БЫСТРОГО ТЕСТА ==========

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "RU");
    bool start_test = false;


    // ===== РЕЖИМ ГЕНЕРАЦИИ =====
    for (int i = 1; i < argc; i++) {
        if (string(argv[i]) == "--generate") {
            generateJSON(argc, argv);
            return 0;
        }
    }

    // Переменные для параметров
    string input_path = "";
    int top_count = 5;

    // Читаем параметры командной строки
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            cout << endl;
            cout << "Программа для анализа продаж" << endl;
            cout << endl;
            cout << "Использование:" << endl;
            cout << "  ./sales --input файл.json       # Читать один файл" << endl;
            cout << "  ./sales --input директория/     # Читать все файлы из директории" << endl;
            cout << "  ./sales -i путь --top 10        # С топ-10 товаров" << endl;
            cout << endl;
            cout << "Опции:" << endl;
            cout << "  -h, --help       Показать справку" << endl;
            cout << "  -i, --input      Файл или директория с данными" << endl;
            cout << "  -t, --top        Сколько товаров показать (по умолчанию 5)" << endl;
            cout << endl;
            cout << "Примеры:" << endl;
            cout << "  ./sales --input data/sales_100.json" << endl;
            cout << "  ./sales --input data/separate_100" << endl;
            cout << "  ./sales --input data/separate_100k --top 20" << endl;
            cout << endl;
            return 0;
        }

        if (arg == "--starttest" || arg == "-st") {
            start_test = true;
        }

        if (arg == "--input" || arg == "-i") {
            if (i + 1 < argc) {
                input_path = argv[i + 1];
                i++;
            }
        }

        if (arg == "--top" || arg == "-t") {
            if (i + 1 < argc) {
                top_count = stoi(argv[i + 1]);
                i++;
            }
        }
    }

    if (start_test) {
        run_benchmark_tests();
        return 0;
    }


    // Проверяем, что указан файл или директория
    if (input_path.empty()) {
        cout << "Ошибка: не указан файл или директория с данными" << endl;
        cout << "Используйте: ./sales --input <файл_или_директория>" << endl;
        return 1;
    }

    cout << endl;
    print_line(70);
    cout << "     АНАЛИЗ ПРОДАЖ" << endl;
    print_line(70);
    cout << endl;

    // ШАГ 1: Загружаем данные
    cout << "Шаг 1: Загрузка из " << input_path << "..." << endl;

    auto time_start = chrono::high_resolution_clock::now();

    vector<Order> orders;

    // Определяем, это файл или директория
    if (is_directory(input_path)) {
        cout << "Режим: чтение директории" << endl;
        orders = read_directory(input_path);
    } else {
        cout << "Режим: чтение одного файла" << endl;

        ifstream file(input_path);
        if (!file.is_open()) {
            cout << "Ошибка: не могу открыть файл" << endl;
            return 1;
        }

        string json_text = "";
        string line;
        while (getline(file, line)) {
            json_text += line;
        }
        file.close();

        orders = read_json(json_text);
    }

    auto time_end = chrono::high_resolution_clock::now();
    int load_time = chrono::duration_cast<chrono::milliseconds>(time_end - time_start).count();

    cout << "  Загружено заказов: " << orders.size() << " за " << load_time << " мс" << endl;

    if (orders.empty()) {
        cout << "Ошибка: не удалось загрузить данные" << endl;
        return 1;
    }

    // ШАГ 2: Проверяем данные
    cout << "\nШаг 2: Проверка данных..." << endl;

    time_start = chrono::high_resolution_clock::now();

    bool data_ok = check_orders(orders);

    time_end = chrono::high_resolution_clock::now();
    int check_time = chrono::duration_cast<chrono::milliseconds>(time_end - time_start).count();

    cout << "  Проверка заняла " << check_time << " мс" << endl;

    if (!data_ok) {
        cout << "\nОшибка: в данных есть ошибки, анализ остановлен" << endl;
        return 1;
    }

    // ШАГ 3: Делаем расчеты
    cout << "\nШаг 3: Анализ данных..." << endl;

    time_start = chrono::high_resolution_clock::now();

    // Считаем общую статистику
    double total_revenue = 0;
    int total_items = 0;

    for (int i = 0; i < orders.size(); i++) {
        total_revenue += calculate_order_total(orders[i]);
        total_items += orders[i].items.size();
    }

    double average_check = calculate_average_check(orders);

    // Выводим общую статистику
    print_header("ОБЩАЯ СТАТИСТИКА");

    cout << "Всего заказов:        " << orders.size() << endl;
    cout << "Общая выручка:        "; print_money(total_revenue); cout << endl;
    cout << "Средний чек:          "; print_money(average_check); cout << endl;
    cout << "Всего товаров:        " << total_items << endl;
    cout << endl;

    // Считаем выручку по дням
    map<string, double> daily = calculate_daily_revenue(orders);

    print_header("ВЫРУЧКА ПО ДНЯМ");

    cout << "Дата            Выручка" << endl;
    cout << "--------------------------------" << endl;

    for (map<string, double>::iterator it = daily.begin(); it != daily.end(); ++it) {
        cout << it->first << "      ";
        print_money(it->second);
        cout << endl;
    }

    cout << endl;

    // Находим топ товаров
    vector<pair<string, double>> top_products = find_top_products(orders, top_count);

    print_header("ТОП ТОВАРОВ ПО ВЫРУЧКЕ");

    cout << "№   Артикул          Выручка" << endl;
    cout << "------------------------------------" << endl;

    for (int i = 0; i < top_products.size(); i++) {
        cout << (i + 1) << ".  " << top_products[i].first << "        ";
        print_money(top_products[i].second);
        cout << endl;
    }

    cout << endl;

    time_end = chrono::high_resolution_clock::now();
    int calc_time = chrono::duration_cast<chrono::milliseconds>(time_end - time_start).count();

    // Итоговое время
    int total_time = load_time + check_time + calc_time;

    print_header("ВРЕМЯ РАБОТЫ");

    cout << "Загрузка данных:  " << load_time << " мс" << endl;
    cout << "Проверка данных:  " << check_time << " мс" << endl;
    cout << "Расчеты:          " << calc_time << " мс" << endl;
    cout << "--------------------------------" << endl;
    cout << "ВСЕГО:            " << total_time << " мс" << endl;
    cout << endl;

    cout << "Готово!" << endl;
    cout << endl;

    return 0;
}