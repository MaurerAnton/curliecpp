// curliecpp - C++ port of curlie (curl + httpie-style output)
// Wraps curl, formats output with ANSI colors and JSON pretty-printing.

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <regex>
#include <unistd.h>
#include <sys/ioctl.h>

static const char* VERSION = "0.1.0";

/* ---- ANSI Colors ---- */
struct ColorScheme {
    const char* deflt  = "\033[37m";
    const char* comment = "\033[90m";
    const char* status  = "\033[33m";
    const char* field   = "\033[34m";
    const char* value   = "\033[36m";
    const char* literal = "\033[35m";
    const char* error   = "\033[31m";
    const char* rst     = "\033[0m";
    bool noColor = false;
};

/* ---- Terminal detection ---- */
static bool isTerminal(int fd) {
    return isatty(fd);
}

/* ---- JSON pretty-printer with ANSI colors ---- */
static std::string prettyJSON(const std::string& raw, const ColorScheme& cs) {
    if (raw.empty()) return raw;
    if (raw[0] != '{' && raw[0] != '[') return raw; /* not JSON */

    std::string out;
    int level = 0;
    char lastQuote = 0;
    bool isValue = false;
    char last = 0;

    for (size_t i = 0; i < raw.size(); i++) {
        char b = raw[i];

        if (last == '\\') { out += b; last = b; continue; }

        if (b == '\'' || b == '"') {
            if (lastQuote == 0) {
                lastQuote = b;
                out += isValue ? cs.value : cs.field;
                out += b;
            } else if (lastQuote == b) {
                lastQuote = 0;
                out += b;
            }
            continue;
        }
        if (lastQuote != 0) { out += b; last = b; continue; }

        /* Outside strings */
        if (b == ' ' || b == '\t' || b == '\r' || b == '\n') continue;

        switch (b) {
            case '{': case '[':
                isValue = false; level++;
                out += cs.deflt; out += b; out += '\n';
                for (int j = 0; j < level; j++) out += "    ";
                break;
            case '}': case ']':
                level--; if (level < 0) level = 0;
                out += '\n';
                for (int j = 0; j < level; j++) out += "    ";
                out += cs.deflt; out += b;
                break;
            case ':':
                isValue = true;
                out += cs.deflt; out += b; out += ' ';
                break;
            case ',':
                isValue = false;
                out += cs.deflt; out += b; out += '\n';
                for (int j = 0; j < level; j++) out += "    ";
                break;
            default:
                if (last == ':') {
                    switch (b) {
                        case 'n': case 't': case 'f': out += cs.literal; break; /* null/true/false */
                        default: out += cs.value; break; /* numbers */
                    }
                }
                out += b;
        }
        last = b;
    }
    out += cs.rst;
    out += '\n';
    return out;
}

/* ---- Header colorizer ---- */
static std::string colorizeHeaders(const std::string& raw, const ColorScheme& cs) {
    std::string out;
    std::string line;
    for (size_t i = 0; i < raw.size(); i++) {
        line += raw[i];
        if (raw[i] == '\n' || i == raw.size() - 1) {
            /* Colorize based on pattern */
            std::regex reStatus("^(HTTP)(/)([\\d.]+\\s+\\d{3})(\\s+.+)");
            std::regex reMethod("^([A-Z]+)(\\s+\\S+\\s+)(HTTP)");
            std::regex reHeader("^([a-zA-Z0-9.-]*?:)(.*)");
            std::regex reCurlErr("^(curl: \\(\\d+\\).*)");
            std::regex reComment("^(\\* .*)");
            std::smatch m;

            if (std::regex_match(line, m, reCurlErr)) {
                out += std::string(cs.error) + m[1].str() + cs.rst + "\n";
            } else if (std::regex_match(line, m, reMethod) && m.size() >= 4) {
                out += std::string(cs.field) + m[1].str() + cs.deflt + m[2].str() + cs.field + m[3].str() + cs.rst + "\n";
            } else if (std::regex_match(line, m, reStatus) && m.size() >= 5) {
                out += std::string(cs.field) + m[1].str() + cs.deflt + m[2].str() + cs.value + m[3].str() + cs.status + m[4].str() + cs.rst + "\n";
            } else if (std::regex_match(line, m, reHeader) && m.size() >= 3) {
                out += std::string(cs.deflt) + m[1].str() + cs.value + m[2].str() + cs.rst + "\n";
            } else if (std::regex_match(line, m, reComment)) {
                out += std::string(cs.comment) + m[1].str() + cs.rst + "\n";
            } else {
                out += line; /* if no newline was added, add one */
                if (line.back() != '\n') out += '\n';
            }
            line.clear();
        }
    }
    return out;
}

/* ---- Main ---- */

static void printHelp(const char* name) {
    printf("curliecpp %s - C++ port of curlie\n", VERSION);
    printf("A frontend to curl that adds the ease of use of httpie.\n\n");
    printf("Usage: %s [curl options] [URL]\n\n", name);
    printf("Additional flags:\n");
    printf("  --pretty       Force pretty output even when piped\n");
    printf("  --curl         Print the equivalent curl command\n");
    printf("  version        Print version\n");
    printf("\nAll other flags are passed directly to curl.\n");
}

int main(int argc, char* argv[]) {
    if (argc >= 2 && strcmp(argv[1], "version") == 0) {
        printf("curliecpp %s\n", VERSION);
        return 0;
    }
    if (argc >= 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        printHelp(argv[0]);
        return 0;
    }

    /* Build curl command */
    std::vector<std::string> curlArgs = {"curl"};
    bool pretty = false;
    bool showCurl = false;
    bool isForm = false;

    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--pretty") { pretty = true; continue; }
        if (a == "--curl") { showCurl = true; continue; }
        if (a == "-F") isForm = true;
        curlArgs.push_back(a);
    }

    /* Default to silent mode (no progress bar) */
    curlArgs.push_back("-s"); curlArgs.push_back("-S");

    /* Empty args → show help */
    if (curlArgs.size() == 3) {
        curlArgs.push_back("-h"); curlArgs.push_back("all");
    }

    /* Show curl command */
    if (showCurl) {
        for (auto& a : curlArgs) {
            printf("%s ", a.c_str());
        }
        printf("\n");
        return 0;
    }

    /* Detect terminal */
    bool isStdoutTerm = isTerminal(STDOUT_FILENO);
    bool isStderrTerm = isTerminal(STDERR_FILENO);
    ColorScheme cs;

    /* Build curl command with output capture */
    std::string cmd;
    bool verbose = false;
    for (auto& a : curlArgs) {
        if (a == "-v" || a == "--verbose") verbose = true;
        /* escape for shell */
        if (a.find('\'') != std::string::npos || a.find('"') != std::string::npos) {
            cmd += " '" + a + "'";
        } else {
            cmd += " '" + a + "'";
        }
    }

    /* If stdout is terminal, pretty-print JSON */
    std::string fullCmd;
    if (pretty || isStdoutTerm) {
        /* Capture stdout + stderr */
        fullCmd = cmd + " 2>&1";
    } else {
        fullCmd = cmd + " 2>/dev/null";
    }

    FILE* f = popen(fullCmd.c_str(), "r");
    if (!f) {
        fprintf(stderr, "Failed to run curl\n");
        return 1;
    }

    /* Read all output */
    std::string output;
    char buf[4096];
    while (fgets(buf, sizeof(buf), f)) output += buf;
    int status = pclose(f);

    if (output.empty()) return WEXITSTATUS(status);

    if (pretty || isStdoutTerm) {
        /* Split headers from body: headers end at \r\n\r\n */
        size_t headerEnd = output.find("\r\n\r\n");
        if (headerEnd == std::string::npos) headerEnd = output.find("\n\n");

        if (headerEnd != std::string::npos) {
            std::string headers = output.substr(0, headerEnd);
            std::string body = output.substr(headerEnd + ((output[headerEnd] == '\r') ? 4 : 2));

            /* Colorize headers to stderr */
            if (verbose && isStderrTerm) {
                fprintf(stderr, "%s", colorizeHeaders(headers, cs).c_str());
            }

            /* Pretty-print body to stdout */
            if (pretty || isStdoutTerm) {
                fprintf(stdout, "%s", prettyJSON(body, cs).c_str());
            } else {
                fprintf(stdout, "%s", body.c_str());
            }
        } else {
            fprintf(stdout, "%s", output.c_str());
        }
    } else {
        fprintf(stdout, "%s", output.c_str());
    }

    return WEXITSTATUS(status);
}
