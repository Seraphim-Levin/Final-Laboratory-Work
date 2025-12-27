#include "../include/generate_data_mf.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <sys/stat.h>
#include <sys/types.h>

using namespace std;

// Генератор случайных чисел
random_device rd;
mt19937 gen(rd());

// Генератор случайных строк для артикулов
string generate_sku() {
    const string prefixes[] = { "PROD", "SKU", "ITEM", "ART", "BOLT" };
    uniform_int_distribution<> prefix_dist(0, 4);
    uniform_int_distribution<> num_dist(100, 999);

    return prefixes[prefix_dist(gen)] + "-" +
           to_string(num_dist(gen)) +
           static_cast<char>('A' + (num_dist(gen) % 26));
}

// Генератор ID заказа
string generate_order_id(int index) {
    ostringstream oss;
    oss << "ORD" << setfill('0') << setw(6) << index;
    return oss.str();
}

// Генератор даты и времени в формате ISO 8601
string generate_timestamp(int days_offset = 0, int hours_offset = 0) {
    time_t now = time(nullptr);
    now += days_offset * 24 * 3600 + hours_offset * 3600;

    tm* tm_info = localtime(&now);

    ostringstream oss;
    oss << (1900 + tm_info->tm_year) << "-"
        << setfill('0') << setw(2) << (1 + tm_info->tm_mon) << "-"
        << setfill('0') << setw(2) << tm_info->tm_mday << "T"
        << setfill('0') << setw(2) << tm_info->tm_hour << ":"
        << setfill('0') << setw(2) << tm_info->tm_min << ":"
        << setfill('0') << setw(2) << tm_info->tm_sec << "Z";

    return oss.str();
}

// Экранирование строк для JSON
string escape_json_string(const string& str) {
    string result = "";
    for (char c : str) {
        if (c == '"') result += "\\\"";
        else if (c == '\\') result += "\\\\";
        else if (c == '\n') result += "\\n";
        else if (c == '\t') result += "\\t";
        else result += c;
    }
    return result;
}

// Генерация одного товара
string generate_item(bool with_errors = false) {
    uniform_real_distribution<> price_dist(10.0, 5000.0);
    uniform_int_distribution<> qty_dist(1, 50);

    string sku = generate_sku();
    int qty = qty_dist(gen);
    double price = price_dist(gen);

    // Намеренные ошибки для тестирования
    if (with_errors) {
        uniform_int_distribution<> error_type(0, 3);
        int error = error_type(gen);

        if (error == 0) {
            sku = "";  // Пустой артикул
        }
        else if (error == 1) {
            qty = 0;  // Нулевое количество
        }
        else if (error == 2) {
            qty = -5;  // Отрицательное количество
        }
        else if (error == 3) {
            price = -100.0;  // Отрицательная цена
        }
    }

    ostringstream oss;
    oss << fixed << setprecision(2);
    oss << "{";
    oss << "\"sku\":\"" << escape_json_string(sku) << "\",";
    oss << "\"qty\":" << qty << ",";
    oss << "\"price\":" << price;
    oss << "}";

    return oss.str();
}

// Генерация одного заказа
string generate_order(int index, bool with_errors = false) {
    uniform_int_distribution<> items_count_dist(1, 10);
    uniform_int_distribution<> days_dist(-30, 0);
    uniform_int_distribution<> hours_dist(0, 23);

    int items_count = items_count_dist(gen);

    // Намеренные ошибки для тестирования
    if (with_errors) {
        uniform_int_distribution<> error_type(0, 2);
        int error = error_type(gen);

        if (error == 0) {
            items_count = 0;  // Нет товаров в заказе
        }
    }

    ostringstream oss;
    oss << "{";
    oss << "\"id\":\"" << generate_order_id(index) << "\",";
    oss << "\"ts\":\"" << generate_timestamp(days_dist(gen), hours_dist(gen)) << "\",";
    oss << "\"items\":[";

    for (int i = 0; i < items_count; i++) {
        if (i > 0) oss << ",";
        oss << generate_item(with_errors);
    }

    oss << "]}";

    return oss.str();
}

// Создать директорию (кросс-платформенно)
bool create_directory(const std::string& path) {
#ifdef _WIN32
    // В Windows (MinGW) используем mkdir с одним аргументом
    return mkdir(path.c_str()) == 0 || errno == EEXIST;
#else
    // В Linux/macOS — с правами доступа
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

// Генерация отдельных JSON файлов
void generate_separate_files(const string& base_dir, int count, bool with_errors) {
    cout << "Генерация отдельных JSON файлов" << endl;
    cout << "Директория: " << base_dir << endl;
    cout << "Количество файлов: " << count << endl;
    cout << "Режим: " << (with_errors ? "с ошибками" : "корректные данные") << endl;
    cout << endl;

    // Создаём базовую директорию
    if (!create_directory(base_dir)) {
        cerr << "Ошибка: не могу создать директорию " << base_dir << endl;
        return;
    }

    auto start_time = chrono::high_resolution_clock::now();

    // Генерируем файлы
    for (int i = 1; i <= count; i++) {
        ostringstream filename;
        filename << base_dir << "/order_" << setfill('0') << setw(6) << i << ".json";

        ofstream file(filename.str());
        if (!file.is_open()) {
            cerr << "Ошибка: не могу создать файл " << filename.str() << endl;
            continue;
        }

        file << "[\n";
        file << generate_order(i, with_errors);
        file << "\n]";
        file.close();

        // Прогресс
        if (count >= 100 && i % (count / 10) == 0) {
            cout << "  Прогресс: " << (i * 100 / count) << "% (" << i << "/" << count << ")" << endl;
        }
    }

    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();

    cout << endl;
    cout << "Создано файлов: " << count << endl;
    cout << "Время генерации: " << duration << " мс (" << (duration / 1000.0) << " сек)" << endl;
    cout << "Скорость: " << (count * 1000.0 / duration) << " файлов/сек" << endl;
    cout << endl;
}

void print_help() {
    cout << endl;

    cout << "  ГЕНЕРАТОР ОТДЕЛЬНЫХ JSON ФАЙЛОВ" << endl;

    cout << endl;
    cout << "Использование:" << endl;
    cout << "  ./src/generate_data --output <dir> --count <N> [--errors]" << endl;
    cout << "  ./src/generate_data --preset <тип>" << endl;
    cout << endl;
    cout << "Основные параметры:" << endl;
    cout << "  --output DIR    - Директория для файлов" << endl;
    cout << "  --count N       - Количество файлов" << endl;
    cout << "  --errors        - Включить генерацию ошибок" << endl;
    cout << endl;
    cout << "Пресеты:" << endl;
    cout << "  small           - 100 файлов" << endl;
    cout << "  medium          - 1,000 файлов" << endl;
    cout << "  large           - 10,000 файлов" << endl;
    cout << "  huge            - 250,000 файлов (ВНИМАНИЕ: долго!)" << endl;
    cout << "  errors          - 1000 файлов с ошибками" << endl;
    cout << endl;
    cout << "Примеры:" << endl;
    cout << "  ./src/generate_data --preset small" << endl;
    cout << "  ./src/generate_data --output data/orders --count 500" << endl;
    cout << "  ./src/generate_data --output data/bad --count 100 --errors" << endl;
    cout << endl;
    cout << "ВАЖНО:" << endl;
    cout << "  100,000 файлов займёт ~30-50 МБ места на диске" << endl;
    cout << "  Генерация может занять 5-20 минут в зависимости от системы" << endl;
    cout << endl;
}

void generateJSON(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        return;
    }

    string output_dir = "";
    int count = 0;
    bool with_errors = false;
    string preset = "";

    // Парсинг аргументов
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            print_help();
            return;
        }
        else if (arg == "--output" || arg == "-o") {
            if (i + 1 < argc) {
                output_dir = argv[i + 1];
                i++;
            }
        }
        else if (arg == "--count" || arg == "-c") {
            if (i + 1 < argc) {
                count = stoi(argv[i + 1]);
                i++;
            }
        }
        else if (arg == "--errors") {
            with_errors = true;
        }
        else if (arg == "--preset" || arg == "-p") {
            if (i + 1 < argc) {
                preset = argv[i + 1];
                i++;
            }
        }
    }

    cout << endl;

    cout << "  ГЕНЕРАТОР ОТДЕЛЬНЫХ JSON ФАЙЛОВ" << endl;

    cout << endl;

    // Обработка пресетов
    if (!preset.empty()) {
        if (preset == "small") {
            generate_separate_files("data/separate_100", 100, false);
        }
        else if (preset == "medium") {
            generate_separate_files("data/separate_1k", 1000, false);
        }
        else if (preset == "large") {
            generate_separate_files("data/separate_10k", 10000, false);
        }
        else if (preset == "huge") {
            cout << "ВНИМАНИЕ: Генерация 250,000 файлов!" << endl;
            cout << "Это займёт ~30-50 МБ места и 5-20 минут времени." << endl;
            cout << "Продолжить? (y/n): ";
            string answer;
            cin >> answer;
            if (answer == "y" || answer == "Y" || answer == "yes") {
                generate_separate_files("data/separate_250k", 250000, false);
            }
            else {
                cout << "Отменено." << endl;
            }
        }
        else if (preset == "errors") {
            generate_separate_files("data/separate_errors", 1000, true);
        }
        else {
            cerr << "Неизвестный пресет: " << preset << endl;
            return;
        }
    }
        // Кастомные параметры
    else if (!output_dir.empty() && count > 0) {
        generate_separate_files(output_dir, count, with_errors);
    }
    else {
        cerr << "Ошибка: укажите --output и --count или используйте --preset" << endl;
        print_help();
        return;
    }


    cout << "  ГЕНЕРАЦИЯ ЗАВЕРШЕНА" << endl;

    cout << endl;
}
