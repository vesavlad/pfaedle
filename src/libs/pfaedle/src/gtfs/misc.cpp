#include <pfaedle/gtfs/misc.h>
#include <pfaedle/gtfs/exceptions.h>
namespace pfaedle::gtfs
{
std::string append_leading_zero(const std::string& s, bool check)
{
    if (check && s.size() > 2)
        throw invalid_field_format("The string for appending zero is too long: " + s);

    if (s.size() == 2)
        return s;
    return "0" + s;
}
std::string add_trailing_slash(const std::string& path)
{
    auto extended_path = path;
    if (!extended_path.empty() && extended_path.back() != '/')
        extended_path += "/";
    return extended_path;
}
void write_joined(std::ofstream& out, std::vector<std::string>&& elements)
{
    for (size_t i = 0; i < elements.size(); ++i)
    {
        out << elements[i];
        if (i != elements.size() - 1)
            out << csv_separator;
    }
    out << std::endl;
}
std::string unquote_text(const std::string& text)
{
    std::string res;
    bool prev_is_quote = false;
    bool prev_is_skipped = false;

    size_t start_index = 0;
    size_t end_index = text.size();

    // Field values that contain quotation marks or commas must be enclosed within quotation marks.
    if (text.size() > 1 && text.front() == quote && text.back() == quote)
    {
        ++start_index;
        --end_index;
    }

    // In addition, each quotation mark in the field value must be preceded with a quotation mark.
    for (size_t i = start_index; i < end_index; ++i)
    {
        if (text[i] != quote)
        {
            res += text[i];
            prev_is_quote = false;
            prev_is_skipped = false;
            continue;
        }

        if (prev_is_quote)
        {
            if (prev_is_skipped)
                res += text[i];

            prev_is_skipped = !prev_is_skipped;
        }
        else
        {
            prev_is_quote = true;
            res += text[i];
        }
    }

    return res;
}
std::string quote_text(const std::string& text)
{
    std::stringstream stream;
    stream << std::quoted(text, quote, quote);
    return stream.str();
}
std::string wrap(const std::string& text)
{
    static const std::string symbols = std::string(1, quote) + std::string(1, csv_separator);

    if (text.find_first_of(symbols) == std::string::npos)
        return text;

    return quote_text(text);
}
std::string wrap(double val)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(6);
    stream << val;
    return stream.str();
}
std::string trim_spaces(const std::string& token)
{
    static const std::string delimiters = " \t";
    std::string res = token;
    res.erase(0, res.find_first_not_of(delimiters));
    res.erase(res.find_last_not_of(delimiters) + 1);
    return res;
}
std::string normalize(std::string& token, bool has_quotes)
{
    std::string res = trim_spaces(token);
    if (has_quotes)
        return unquote_text(res);
    return res;
}
std::set<route_type> get_route_types_from_string(std::string name)
{
    std::set<route_type> ret;

    if (name.empty())
        return ret;

    char *rem;
    uint64_t num = strtol(name.c_str(), &rem, 10);
    if (!*rem) {
        auto i = get_route_type(num);
        ret.insert(i);
        return ret;
    }

    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    if (name == "all") {
        ret.insert(route_type::Tram);
        ret.insert(route_type::Subway);
        ret.insert(route_type::Rail);
        ret.insert(route_type::Bus);
        ret.insert(route_type::CoachService);
        ret.insert(route_type::Ferry);
        ret.insert(route_type::CableTram);
        ret.insert(route_type::AerialLift);
        ret.insert(route_type::Funicular);
        ret.insert(route_type::Trolleybus);
        return ret;
    }

    if (name == "bus") {
        ret.insert(route_type::Bus);
        return ret;
    }

    if (name == "trolley" || name == "trolleybus" || name == "trolley-bus") {
        ret.insert(route_type::Trolleybus);
        return ret;
    }

    if (name == "tram" || name == "streetcar" || name == "light_rail" ||
        name == "lightrail" || name == "light-rail") {
        ret.insert(route_type::Tram);
        return ret;
    }

    if (name == "train" || name == "rail") {
        ret.insert(route_type::Rail);
        return ret;
    }

    if (name == "ferry" || name == "boat" || name == "ship") {
        ret.insert(route_type::Ferry);
        return ret;
    }

    if (name == "subway" || name == "metro") {
        ret.insert(route_type::Subway);
        return ret;
    }

    if (name == "cablecar" || name == "cable_car" || name == "cable-car") {
        ret.insert(route_type::CableTram);
        return ret;
    }

    if (name == "gondola") {
        ret.insert(route_type::AerialLift);
        return ret;
    }

    if (name == "funicular") {
        ret.insert(route_type::Funicular);
        return ret;
    }

    if (name == "coach") {
        ret.insert(route_type::CoachService);
        return ret;
    }

    return ret;
}
std::string get_hex_color_string(uint32_t color)
{
    // using stringstream here, because it doesnt add "0x" to the front
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(6) << color;
    return ss.str();
}
std::string get_route_type_string(route_type t)
{
    if (t == route_type::CoachService)
        return "coach";

    static std::string names[12] = {"tram", "subway", "rail", "bus", "ferry", "cablecar", "gondola", "funicular", "", "", "", "trolleybus"};
    return names[static_cast<size_t>(t)];
}
route_type get_route_type(int t)
{
    switch (t) {
        case 2:
        case 100:
        case 101:
        case 102:
        case 103:
        case 104:
        case 105:
        case 106:
        case 107:
        case 108:
        case 109:
        case 110:
        case 111:
        case 112:
        case 113:
        case 114:
        case 115:
        case 117:
        case 300:
        case 400:
        case 403:
        case 404:
        case 405:
        case 1503:
            return route_type::Rail;
        case 3:
            return route_type::Bus;
        case 200:
        case 201:
        case 202:
        case 203:
        case 204:
        case 205:
        case 206:
        case 207:
        case 208:
        case 209:
            return route_type::CoachService;
        case 700:
        case 701:
        case 702:
        case 703:
        case 704:
        case 705:
        case 706:
        case 707:
        case 708:
        case 709:
        case 710:
        case 711:
        case 712:
        case 713:
        case 714:
        case 715:
        case 716:
        case 717:
        case 800:
        case 1500:
        case 1501:
        case 1505:
        case 1506:
        case 1507:
            return route_type::Bus;
        case 1:
        case 401:
        case 402:
        case 500:
        case 600:
            return route_type::Subway;
        case 0:
        case 900:
        case 901:
        case 902:
        case 903:
        case 904:
        case 905:
        case 906:
            return route_type::Tram;
            // TODO(patrick): from here on not complete!
        case 4:
        case 1000:
        case 1200:
        case 1502:
            return route_type::Ferry;
        case 6:
        case 1300:
        case 1301:
        case 1304:
        case 1306:
        case 1307:
            return route_type::AerialLift;
        case 7:
        case 116:
        case 1303:
        case 1302:
        case 1400:
            return route_type::Funicular;
        case 5:
            return route_type::CableTram;
        case 11:
            return route_type::Trolleybus;
        default:
            throw invalid_field_format("invalid route type provided");
    }
}
}
