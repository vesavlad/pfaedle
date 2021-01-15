#include <pfaedle/gtfs/access/csv_parser.h>
namespace pfaedle::gtfs::access
{
csv_parser::csv_parser(const std::string & gtfs_directory) : gtfs_path(gtfs_directory) {}

std::vector<std::string> csv_parser::split_record(const std::string & record, bool is_header)
{
    size_t start_index = 0;
    if (is_header)
    {
        // ignore UTF-8 BOM prefix:
        if (record.size() > 2 && record[0] == '\xef' && record[1] == '\xbb' && record[2] == '\xbf')
            start_index = 3;
    }

    std::vector<std::string> fields;
    fields.reserve(20);

    std::string token;
    token.reserve(record.size());

    bool is_inside_quotes = false;
    bool quotes_in_token = false;

    for (size_t i = start_index; i < record.size(); ++i)
    {
        if (record[i] == quote)
        {
            is_inside_quotes = !is_inside_quotes;
            quotes_in_token = true;
            token += record[i];
            continue;
        }

        if (record[i] == csv_separator)
        {
            if (is_inside_quotes)
            {
                token += record[i];
                continue;
            }

            fields.emplace_back(normalize(token, quotes_in_token));
            token.clear();
            quotes_in_token = false;
            continue;
        }

        // Skip delimiters:
        if (record[i] != '\t' && record[i] != '\r')
            token += record[i];
    }

    fields.emplace_back(normalize(token, quotes_in_token));
    return fields;
}

result csv_parser::read_header(const std::string & csv_filename)
{
    if (csv_stream.is_open())
        csv_stream.close();

    csv_stream.open(gtfs_path + csv_filename);
    if (!csv_stream.is_open())
        return {result_code::ERROR_FILE_ABSENT, "File " + csv_filename + " could not be opened"};

    std::string header;
    if (!getline(csv_stream, header) || header.empty())
        return {result_code::ERROR_INVALID_FIELD_FORMAT, "Empty header in file " + csv_filename};

    field_sequence = split_record(header, true);
    return result_code::OK;
}

result csv_parser::read_row(std::map<std::string, std::string> & obj)
{
    obj = {};
    std::string row;
    if (!getline(csv_stream, row))
        return {result_code::END_OF_FILE, {}};

    if (row == "\r")
        return result_code::OK;

    const std::vector<std::string> fields_values = split_record(row);

    // Different count of fields in the row and in the header of csv.
    // Typical approach is to skip not required fields.
    const size_t fields_count = std::min(field_sequence.size(), fields_values.size());

    for (size_t i = 0; i < fields_count; ++i)
        obj[field_sequence[i]] = fields_values[i];

    return result_code::OK;
}
}
