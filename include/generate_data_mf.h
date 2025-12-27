#pragma once
#include <string>

using namespace std;

// Генерация отдельных JSON файлов
void generate_separate_files(const string& base_dir, int count, bool with_errors = false);

// CLI-генератор (если нужен)
void generateJSON(int argc, char* argv[]);