#pragma once
#include <string>
namespace pfaedle::gtfs
{
// Custom types for GTFS fields --------------------------------------------------------------------
// Id of GTFS entity, a sequence of any UTF-8 characters. Used as type for ID GTFS fields.
using Id = std::string;
// A string of UTF-8 characters. Used as type for Text GTFS fields.
using Text = std::string;

// An ISO 4217 alphabetical currency code. Used as type for Currency Code GTFS fields.
using CurrencyCode = std::string;
// An IETF BCP 47 language code. Used as type for Language Code GTFS fields.
using LanguageCode = std::string;

using Message = std::string;
}
