//
// split an image composed of multiple 
// parts into individual texture files
//

#include <iostream>
#include <map>
#include <vector>
#include <stdexcept>
#include <string>

// for reading in a given texture. this does, however, introduce a dependency on SDL
#include "../libs/texture.h"

using namespace std;

struct arg_info {
    string filename;
    string output_location;
    int images_vertical;
    int images_horizontal;
};

// functions used within main
void usage(const char* prog_name);
auto parse_input_arguments(int argc, char** argv) -> std::map<std::string, std::string>;
auto convert_input_arguments(map<string, string>& args) -> arg_info;

int main(int argc, char* argv[]) {

    auto args   = parse_input_arguments(argc, argv);
    auto inputs = convert_input_arguments(args);
    if(args.size() != 4) usage(argv[0]);

    



    return 0;
}

auto convert_input_arguments(map<string, string>& args) -> arg_info {

    arg_info ai;

    auto iter = args.find("file");
    if(iter != args.end()) {
        ai.filename = iter->second;
    }
    else {
        throw std::runtime_error("expoecting --file=<> argument");
    }

    auto iter = args.find("height");
    if(iter != args.end()) {
        ai.images_horizontal = stoi(iter->second);
    }
    else {
        throw std::runtime_error("expoecting --height=<> argument");
    }

    auto iter = args.find("width");
    if(iter != args.end()) {
        ai.images_vertical = stoi(iter->second);
    }
    else {
        throw std::runtime_error("expoecting --width=<> argument");
    }

    auto iter = args.find("output");
    if(iter != args.end()) {
        ai.filename = iter->second;
    }
    else {
        throw std::runtime_error("expoecting --output=<> argument");
    }

    return ai;
}

auto parse_input_arguments(int argc, char** argv) -> std::map<std::string, std::string> {

    map<string, string> args;

    // fill vector with data. easier to iterate through than
    vector<string> v;
    for(int i = 1; i < argc; i++)
        v.push_back({ argv[i] });

    for(auto s : v) {

        const int state_expect_dash_0 = 0;
        const int state_expect_dash_1 = 1;
        const int state_key           = 2;
        const int state_value         = 3;

        int state_current = state_expect_dash_0;

        string key;
        string value;

        for(auto c : s) {

            switch(state_current) {
                case state_expect_dash_0:
                    if(c == '-') {
                        state_current = state_expect_dash_1;
                    }
                    else {
                        cerr << "expecting first '-' in argument '" << s << "'" << endl;
                        exit(1);
                    }
                    break;
                case state_expect_dash_1:
                    if(c == '-') {
                        state_current = state_key;
                    }
                    else {
                        cerr << "expecting second '-' in argument '" << s << "'" << endl;
                        exit(1);
                    }
                    break;
                case state_key:
                    if(c == '=') {
                        state_current = state_value;
                    }
                    else {
                        key.push_back(c);
                    }
                    break;
                case state_value:
                    value.push_back(c);
                    break;
                default:
                    cerr << "unknown internal error...\n";
                    exit(1);
            }

        }

        if(key.size() == 0) {
            cerr << "key in argument '" << s << "' must be at least one character" << endl;
            exit(1);
        }

        if(value.size() == 0) {
            cerr << "value in argument '" << s << "' must be at least one character" << endl;
            exit(1);
        }

        args[key] = value;

    }

    return args;
}

void usage(const char* prog_name) {
    cout << "usage:\n    ";
    cout << prog_name << " --file=<filename> --height=<vertical images> --width=<horizontal images> --output=<output directory>\n\n";
    exit(1);
}
