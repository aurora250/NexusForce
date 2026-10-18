#include "locale_data.hpp"
#ifndef NEFORCE_SUPPORT_ICU
NEFORCE_BEGIN_NAMESPACE__

namespace locale_data {
    namespace {
        constexpr bool same_code(string_view left, string_view right) noexcept {
            if (left.empty() || right.empty()) {
                return false;
            }

            size_t index = 0;
            while (left[index] != '\0' && right[index] != '\0') {
                const char a =
                        (left[index] >= 'A' && left[index] <= 'Z') ? static_cast<char>(left[index] + 32) : left[index];
                const char b = (right[index] >= 'A' && right[index] <= 'Z') ? static_cast<char>(right[index] + 32)
                                                                            : right[index];
                if (a != b) {
                    return false;
                }
                ++index;
            }
            return left[index] == right[index];
        }

        constexpr bool in_code_list(string_view code, const string_view* list, const uint32_t count) noexcept {
            for (uint32_t i = 0; i < count; ++i) {
                if (same_code(code, list[i])) {
                    return true;
                }
            }
            return false;
        }

        // Non-breaking spaces are spelled out as UTF-8 escapes to keep the data file readable
        // in editors that do not render them.
        constexpr string_view NBSP = "\xC2\xA0";
        constexpr string_view NARROW_NBSP = "\xE2\x80\xAF";
        constexpr string_view ARABIC_DECIMAL = "\xD9\xAB";
        constexpr string_view ARABIC_GROUP = "\xD9\xAC";

        constexpr number_style DEFAULT_NUMBER_STYLE = {".", ",", 0, false, true, ""};

        struct number_style_entry {
            string_view language;
            number_style style;
        };

        constexpr string_view NARROW_SPACE = " ";
        constexpr string_view NO_SPACE = "";

        const number_style_entry NUMBER_STYLES[] = {
                // Decimal comma, dot grouping, the currency symbol follows the amount.
                {"de", {",", ".", 0, false, false, NBSP}},
                {"es", {",", ".", 0, false, false, NBSP}},
                {"it", {",", ".", 0, false, false, NBSP}},
                {"pt", {",", ".", 0, false, false, NBSP}},
                {"id", {",", ".", 0, false, true, NO_SPACE}},
                {"da", {",", ".", 0, false, false, NBSP}},
                {"hr", {",", ".", 0, false, false, NBSP}},
                {"sl", {",", ".", 0, false, false, NBSP}},
                {"sr", {",", ".", 0, false, false, NBSP}},
                {"bs", {",", ".", 0, false, false, NBSP}},
                {"mk", {",", ".", 0, false, false, NBSP}},
                {"sq", {",", ".", 0, false, false, NBSP}},
                {"ca", {",", ".", 0, false, false, NBSP}},
                {"gl", {",", ".", 0, false, false, NBSP}},
                {"eu", {",", ".", 0, false, false, NBSP}},
                {"vi", {",", ".", 0, false, false, NBSP}},
                {"is", {",", ".", 0, false, false, NBSP}},
                {"ro", {",", ".", 0, false, false, NBSP}},
                {"tr", {",", ".", 0, false, true, NO_SPACE}},
                {"el", {",", ".", 0, false, false, NBSP}},
                {"nl", {",", ".", 0, false, true, NARROW_SPACE}},
                // Decimal comma, non-breaking space grouping, the currency symbol follows the amount.
                {"fr", {",", NARROW_NBSP, 0, false, false, NBSP}},
                {"ru", {",", NBSP, 0, false, false, NBSP}},
                {"sv", {",", NBSP, 0, false, false, NBSP}},
                {"nb", {",", NBSP, 0, false, false, NBSP}},
                {"nn", {",", NBSP, 0, false, false, NBSP}},
                {"fi", {",", NBSP, 0, false, false, NBSP}},
                {"pl", {",", NBSP, 0, false, false, NBSP}},
                {"cs", {",", NBSP, 0, false, false, NBSP}},
                {"sk", {",", NBSP, 0, false, false, NBSP}},
                {"hu", {",", NBSP, 0, false, false, NBSP}},
                {"uk", {",", NBSP, 0, false, false, NBSP}},
                {"be", {",", NBSP, 0, false, false, NBSP}},
                {"bg", {",", NBSP, 0, false, false, NBSP}},
                {"et", {",", NBSP, 0, false, false, NBSP}},
                {"lv", {",", NBSP, 0, false, false, NBSP}},
                {"lt", {",", NBSP, 0, false, false, NBSP}},
                {"kk", {",", NBSP, 0, false, false, NBSP}},
                {"ky", {",", NBSP, 0, false, false, NBSP}},
                {"az", {",", NBSP, 0, false, false, NBSP}},
                {"uz", {",", NBSP, 0, false, false, NBSP}},
                // Arabic script: arabic-indic or extended arabic-indic digits.
                {"ar", {ARABIC_DECIMAL, ARABIC_GROUP, 0x0660, false, false, NBSP}},
                {"fa", {ARABIC_DECIMAL, ARABIC_GROUP, 0x06F0, false, false, NBSP}},
                {"ur", {ARABIC_DECIMAL, ARABIC_GROUP, 0x0660, false, false, NBSP}},
                {"ps", {ARABIC_DECIMAL, ARABIC_GROUP, 0x0660, false, false, NBSP}},
                {"sd", {ARABIC_DECIMAL, ARABIC_GROUP, 0x0660, false, false, NBSP}},
                {"ckb", {ARABIC_DECIMAL, ARABIC_GROUP, 0x0660, false, false, NBSP}},
                {"dv", {ARABIC_DECIMAL, ARABIC_GROUP, 0x0660, false, false, NBSP}},
                // Indian grouping: the last group holds three digits, every other group two.
                {"hi", {".", ",", 0, true, true, NO_SPACE}},
                {"bn", {".", ",", 0, true, true, NO_SPACE}},
                {"mr", {".", ",", 0, true, true, NO_SPACE}},
                {"ta", {".", ",", 0, true, true, NO_SPACE}},
                {"te", {".", ",", 0, true, true, NO_SPACE}},
                {"kn", {".", ",", 0, true, true, NO_SPACE}},
                {"ml", {".", ",", 0, true, true, NO_SPACE}},
                {"gu", {".", ",", 0, true, true, NO_SPACE}},
                {"pa", {".", ",", 0, true, true, NO_SPACE}},
                {"or", {".", ",", 0, true, true, NO_SPACE}},
                {"as", {".", ",", 0, true, true, NO_SPACE}},
                {"ne", {".", ",", 0, true, true, NO_SPACE}},
        };

        constexpr string_view EN_LONG_DAYS[] = {"Sunday",   "Monday", "Tuesday", "Wednesday",
                                                "Thursday", "Friday", "Saturday"};
        constexpr string_view EN_SHORT_DAYS[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        constexpr string_view EN_LONG_MONTHS[] = {"January",   "February", "March",    "April",
                                                  "May",       "June",     "July",     "August",
                                                  "September", "October",  "November", "December"};
        constexpr string_view EN_SHORT_MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

        constexpr string_view DE_LONG_DAYS[] = {"Sonntag",    "Montag",  "Dienstag", "Mittwoch",
                                                "Donnerstag", "Freitag", "Samstag"};
        constexpr string_view DE_SHORT_DAYS[] = {"So.", "Mo.", "Di.", "Mi.", "Do.", "Fr.", "Sa."};
        constexpr string_view DE_LONG_MONTHS[] = {"Januar", "Februar", "März",      "April",   "Mai",      "Juni",
                                                  "Juli",   "August",  "September", "Oktober", "November", "Dezember"};
        constexpr string_view DE_SHORT_MONTHS[] = {"Jan.", "Feb.", "März",  "Apr.", "Mai",  "Juni",
                                                   "Juli", "Aug.", "Sept.", "Okt.", "Nov.", "Dez."};

        constexpr string_view FR_LONG_DAYS[] = {"dimanche", "lundi",    "mardi", "mercredi",
                                                "jeudi",    "vendredi", "samedi"};
        constexpr string_view FR_SHORT_DAYS[] = {"dim.", "lun.", "mar.", "mer.", "jeu.", "ven.", "sam."};
        constexpr string_view FR_LONG_MONTHS[] = {"janvier", "février", "mars",      "avril",   "mai",      "juin",
                                                  "juillet", "août",    "septembre", "octobre", "novembre", "décembre"};
        constexpr string_view FR_SHORT_MONTHS[] = {"janv.", "févr.", "mars",  "avr.", "mai",  "juin",
                                                   "juil.", "août",  "sept.", "oct.", "nov.", "déc."};

        constexpr string_view ES_LONG_DAYS[] = {"domingo", "lunes",   "martes", "miércoles",
                                                "jueves",  "viernes", "sábado"};
        constexpr string_view ES_SHORT_DAYS[] = {"dom", "lun", "mar", "mié", "jue", "vie", "sáb"};
        constexpr string_view ES_LONG_MONTHS[] = {"enero",      "febrero", "marzo",     "abril",
                                                  "mayo",       "junio",   "julio",     "agosto",
                                                  "septiembre", "octubre", "noviembre", "diciembre"};
        constexpr string_view ES_SHORT_MONTHS[] = {"ene", "feb", "mar",  "abr", "may", "jun",
                                                   "jul", "ago", "sept", "oct", "nov", "dic"};

        constexpr string_view IT_LONG_DAYS[] = {"domenica", "lunedì",  "martedì", "mercoledì",
                                                "giovedì",  "venerdì", "sabato"};
        constexpr string_view IT_SHORT_DAYS[] = {"dom", "lun", "mar", "mer", "gio", "ven", "sab"};
        constexpr string_view IT_LONG_MONTHS[] = {"gennaio",   "febbraio", "marzo",    "aprile",
                                                  "maggio",    "giugno",   "luglio",   "agosto",
                                                  "settembre", "ottobre",  "novembre", "dicembre"};
        constexpr string_view IT_SHORT_MONTHS[] = {"gen", "feb", "mar", "apr", "mag", "giu",
                                                   "lug", "ago", "set", "ott", "nov", "dic"};

        constexpr string_view PT_LONG_DAYS[] = {"domingo",      "segunda-feira", "terça-feira", "quarta-feira",
                                                "quinta-feira", "sexta-feira",   "sábado"};
        constexpr string_view PT_SHORT_DAYS[] = {"dom.", "seg.", "ter.", "qua.", "qui.", "sex.", "sáb."};
        constexpr string_view PT_LONG_MONTHS[] = {"janeiro",  "fevereiro", "março",    "abril",
                                                  "maio",     "junho",     "julho",    "agosto",
                                                  "setembro", "outubro",   "novembro", "dezembro"};
        constexpr string_view PT_SHORT_MONTHS[] = {"jan.", "fev.", "mar.", "abr.", "mai.", "jun.",
                                                   "jul.", "ago.", "set.", "out.", "nov.", "dez."};

        constexpr string_view RU_LONG_DAYS[] = {"воскресенье", "понедельник", "вторник", "среда",
                                                "четверг",     "пятница",     "суббота"};
        constexpr string_view RU_SHORT_DAYS[] = {"вс", "пн", "вт", "ср", "чт", "пт", "сб"};
        constexpr string_view RU_LONG_MONTHS[] = {"январь", "февраль", "март",     "апрель",  "май",    "июнь",
                                                  "июль",   "август",  "сентябрь", "октябрь", "ноябрь", "декабрь"};
        constexpr string_view RU_SHORT_MONTHS[] = {"янв.", "февр.", "мар.",  "апр.", "мая",   "июн.",
                                                   "июл.", "авг.",  "сент.", "окт.", "нояб.", "дек."};

        constexpr string_view ZH_LONG_DAYS[] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};
        constexpr string_view ZH_SHORT_DAYS[] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};
        constexpr string_view ZH_LONG_MONTHS[] = {"一月", "二月", "三月", "四月", "五月",   "六月",
                                                  "七月", "八月", "九月", "十月", "十一月", "十二月"};
        constexpr string_view ZH_SHORT_MONTHS[] = {"1月", "2月", "3月", "4月",  "5月",  "6月",
                                                   "7月", "8月", "9月", "10月", "11月", "12月"};

        constexpr string_view JA_LONG_DAYS[] = {"日曜日", "月曜日", "火曜日", "水曜日", "木曜日", "金曜日", "土曜日"};
        constexpr string_view JA_SHORT_DAYS[] = {"日", "月", "火", "水", "木", "金", "土"};
        constexpr string_view JA_LONG_MONTHS[] = {"1月", "2月", "3月", "4月",  "5月",  "6月",
                                                  "7月", "8月", "9月", "10月", "11月", "12月"};
        constexpr string_view JA_SHORT_MONTHS[] = {"1月", "2月", "3月", "4月",  "5月",  "6月",
                                                   "7月", "8月", "9月", "10月", "11月", "12月"};

        constexpr string_view KO_LONG_DAYS[] = {"일요일", "월요일", "화요일", "수요일", "목요일", "금요일", "토요일"};
        constexpr string_view KO_SHORT_DAYS[] = {"일", "월", "화", "수", "목", "금", "토"};
        constexpr string_view KO_LONG_MONTHS[] = {"1월", "2월", "3월", "4월",  "5월",  "6월",
                                                  "7월", "8월", "9월", "10월", "11월", "12월"};
        constexpr string_view KO_SHORT_MONTHS[] = {"1월", "2월", "3월", "4월",  "5월",  "6월",
                                                   "7월", "8월", "9월", "10월", "11월", "12월"};

        constexpr string_view AR_LONG_DAYS[] = {"الأحد",  "الاثنين", "الثلاثاء", "الأربعاء",
                                                "الخميس", "الجمعة",  "السبت"};
        constexpr string_view AR_SHORT_DAYS[] = {"أحد", "اثنين", "ثلاثاء", "أربعاء", "خميس", "جمعة", "سبت"};
        constexpr string_view AR_LONG_MONTHS[] = {"يناير", "فبراير", "مارس",   "أبريل",  "مايو",   "يونيو",
                                                  "يوليو", "أغسطس",  "سبتمبر", "أكتوبر", "نوفمبر", "ديسمبر"};
        constexpr string_view AR_SHORT_MONTHS[] = {"يناير", "فبراير", "مارس",   "أبريل",  "مايو",   "يونيو",
                                                   "يوليو", "أغسطس",  "سبتمبر", "أكتوبر", "نوفمبر", "ديسمبر"};

        constexpr string_view HI_LONG_DAYS[] = {"रविवार", "सोमवार", "मंगलवार", "बुधवार", "गुरुवार", "शुक्रवार", "शनिवार"};
        constexpr string_view HI_SHORT_DAYS[] = {"रवि", "सोम", "मंगल", "बुध", "गुरु", "शुक्र", "शनि"};
        constexpr string_view HI_LONG_MONTHS[] = {"जनवरी", "फ़रवरी", "मार्च",  "अप्रैल",  "मई",   "जून",
                                                  "जुलाई",  "अगस्त",  "सितंबर", "अक्टूबर", "नवंबर", "दिसंबर"};
        constexpr string_view HI_SHORT_MONTHS[] = {"जन॰", "फ़र॰", "मार्च", "अप्रैल", "मई",  "जून",
                                                   "जुल॰", "अग॰", "सित॰", "अक्तू॰", "नव॰", "दिस॰"};

        const calendar_style CALENDAR_STYLES[] = {
                {"en", EN_LONG_DAYS, EN_SHORT_DAYS, EN_LONG_MONTHS, EN_SHORT_MONTHS, "AM", "PM", "EEEE, MMMM d, y",
                 "MMMM d, y", "MMM d, y", "M/d/yy", "h:mm:ss a z", "h:mm:ss a z", "h:mm:ss a", "h:mm a"},
                {"de", DE_LONG_DAYS, DE_SHORT_DAYS, DE_LONG_MONTHS, DE_SHORT_MONTHS, "AM", "PM", "EEEE, d. MMMM y",
                 "d. MMMM y", "dd.MM.y", "dd.MM.yy", "HH:mm:ss z", "HH:mm:ss z", "HH:mm:ss", "HH:mm"},
                {"fr", FR_LONG_DAYS, FR_SHORT_DAYS, FR_LONG_MONTHS, FR_SHORT_MONTHS, "AM", "PM", "EEEE d MMMM y",
                 "d MMMM y", "d MMM y", "dd/MM/y", "HH:mm:ss z", "HH:mm:ss z", "HH:mm:ss", "HH:mm"},
                {"es", ES_LONG_DAYS, ES_SHORT_DAYS, ES_LONG_MONTHS, ES_SHORT_MONTHS, "a. m.", "p. m.",
                 "EEEE, d 'de' MMMM 'de' y", "d 'de' MMMM 'de' y", "d MMM y", "d/M/yy", "H:mm:ss z", "H:mm:ss z",
                 "H:mm:ss", "H:mm"},
                {"it", IT_LONG_DAYS, IT_SHORT_DAYS, IT_LONG_MONTHS, IT_SHORT_MONTHS, "AM", "PM", "EEEE d MMMM y",
                 "d MMMM y", "d MMM y", "dd/MM/yy", "HH:mm:ss z", "HH:mm:ss z", "HH:mm:ss", "HH:mm"},
                {"pt", PT_LONG_DAYS, PT_SHORT_DAYS, PT_LONG_MONTHS, PT_SHORT_MONTHS, "AM", "PM",
                 "EEEE, d 'de' MMMM 'de' y", "d 'de' MMMM 'de' y", "d 'de' MMM 'de' y", "dd/MM/y", "HH:mm:ss z",
                 "HH:mm:ss z", "HH:mm:ss", "HH:mm"},
                {"ru", RU_LONG_DAYS, RU_SHORT_DAYS, RU_LONG_MONTHS, RU_SHORT_MONTHS, "AM", "PM", "EEEE, d MMMM y 'г'.",
                 "d MMMM y 'г'.", "d MMM y 'г'.", "dd.MM.y", "HH:mm:ss z", "HH:mm:ss z", "HH:mm:ss", "HH:mm"},
                {"zh", ZH_LONG_DAYS, ZH_SHORT_DAYS, ZH_LONG_MONTHS, ZH_SHORT_MONTHS, "上午", "下午", "y年M月d日EEEE",
                 "y年M月d日", "y年M月d日", "y/M/d", "HH:mm:ss z", "HH:mm:ss z", "HH:mm:ss", "HH:mm"},
                {"ja", JA_LONG_DAYS, JA_SHORT_DAYS, JA_LONG_MONTHS, JA_SHORT_MONTHS, "午前", "午後", "y年M月d日EEEE",
                 "y年M月d日", "y/MM/dd", "y/MM/dd", "H:mm:ss z", "H:mm:ss z", "H:mm:ss", "H:mm"},
                {"ko", KO_LONG_DAYS, KO_SHORT_DAYS, KO_LONG_MONTHS, KO_SHORT_MONTHS, "AM", "PM", "y년 M월 d일 EEEE",
                 "y년 M월 d일", "y. M. d.", "yy. M. d.", "a h:mm:ss z", "a h:mm:ss z", "a h:mm:ss", "a h:mm"},
                {"ar", AR_LONG_DAYS, AR_SHORT_DAYS, AR_LONG_MONTHS, AR_SHORT_MONTHS, "ص", "م", "EEEE، d MMMM y",
                 "d MMMM y", "dd/MM/y", "d/M/yy", "h:mm:ss a z", "h:mm:ss a z", "h:mm:ss a", "h:mm a"},
                {"hi", HI_LONG_DAYS, HI_SHORT_DAYS, HI_LONG_MONTHS, HI_SHORT_MONTHS, "पूर्वाह्न", "अपराह्न",
                 "EEEE, d MMMM y", "d MMMM y", "d MMM y", "d/M/yy", "h:mm:ss a z", "h:mm:ss a z", "h:mm:ss a",
                 "h:mm a"},
        };

        // English and Portuguese spread over several date orders, so the region decides:
        // the United States keeps the month first, the Commonwealth and Ireland put the day first,
        // and Australia, India and New Zealand keep the twelve hour clock.
        const calendar_style EN_GB_STYLE = {"en-GB",   EN_LONG_DAYS, EN_SHORT_DAYS,    EN_LONG_MONTHS, EN_SHORT_MONTHS,
                                            "am",      "pm",         "EEEE, d MMMM y", "d MMMM y",     "d MMM y",
                                            "dd/MM/y", "HH:mm:ss z", "HH:mm:ss z",     "HH:mm:ss",     "HH:mm"};
        const calendar_style EN_AU_STYLE = {"en-AU",  EN_LONG_DAYS,  EN_SHORT_DAYS,    EN_LONG_MONTHS, EN_SHORT_MONTHS,
                                            "am",     "pm",          "EEEE, d MMMM y", "d MMMM y",     "d MMM y",
                                            "d/M/yy", "h:mm:ss a z", "h:mm:ss a z",    "h:mm:ss a",    "h:mm a"};
        const calendar_style PT_PT_STYLE = {"pt-PT",
                                            PT_LONG_DAYS,
                                            PT_SHORT_DAYS,
                                            PT_LONG_MONTHS,
                                            PT_SHORT_MONTHS,
                                            "AM",
                                            "PM",
                                            "EEEE, d 'de' MMMM 'de' y",
                                            "d 'de' MMMM 'de' y",
                                            "dd/MM/y",
                                            "dd/MM/y",
                                            "HH:mm:ss z",
                                            "HH:mm:ss z",
                                            "HH:mm:ss",
                                            "HH:mm"};

        struct region_style_entry {
            string_view language;
            string_view country;
            const calendar_style* style;
        };

        constexpr region_style_entry REGION_STYLES[] = {
                {"en", "GB", &EN_GB_STYLE}, {"en", "IE", &EN_GB_STYLE}, {"en", "ZA", &EN_GB_STYLE},
                {"en", "SG", &EN_GB_STYLE}, {"en", "HK", &EN_GB_STYLE}, {"en", "MY", &EN_GB_STYLE},
                {"en", "MT", &EN_GB_STYLE}, {"en", "CY", &EN_GB_STYLE}, {"en", "TT", &EN_GB_STYLE},
                {"en", "JM", &EN_GB_STYLE}, {"en", "KE", &EN_GB_STYLE}, {"en", "NG", &EN_GB_STYLE},
                {"en", "GH", &EN_GB_STYLE}, {"en", "AU", &EN_AU_STYLE}, {"en", "NZ", &EN_AU_STYLE},
                {"en", "IN", &EN_AU_STYLE}, {"en", "PK", &EN_AU_STYLE}, {"en", "BD", &EN_AU_STYLE},
                {"en", "LK", &EN_AU_STYLE}, {"pt", "PT", &PT_PT_STYLE},
        };

        constexpr currency_entry CURRENCIES[] = {
                {"USD", "$", 2},    {"EUR", "€", 2},   {"GBP", "£", 2},   {"JPY", "¥", 0},    {"CNY", "¥", 2},
                {"KRW", "₩", 0},    {"RUB", "₽", 2},   {"INR", "₹", 2},   {"BRL", "R$", 2},   {"AUD", "A$", 2},
                {"CAD", "CA$", 2},  {"CHF", "CHF", 2}, {"HKD", "HK$", 2}, {"TWD", "NT$", 2},  {"SGD", "S$", 2},
                {"NZD", "NZ$", 2},  {"SEK", "kr", 2},  {"NOK", "kr", 2},  {"DKK", "kr", 2},   {"PLN", "zł", 2},
                {"CZK", "Kč", 2},   {"HUF", "Ft", 2},  {"RON", "lei", 2}, {"BGN", "лв", 2},   {"TRY", "₺", 2},
                {"UAH", "₴", 2},    {"KZT", "₸", 2},   {"ZAR", "R", 2},   {"MXN", "MX$", 2},  {"ARS", "AR$", 2},
                {"CLP", "CL$", 0},  {"COP", "CO$", 2}, {"PEN", "S/", 2},  {"THB", "฿", 2},    {"VND", "₫", 0},
                {"IDR", "Rp", 2},   {"MYR", "RM", 2},  {"PHP", "₱", 2},   {"ILS", "₪", 2},    {"SAR", "﷼", 2},
                {"AED", "د.إ", 2},  {"EGP", "E£", 2},  {"NGN", "₦", 2},   {"KES", "KSh", 2},  {"PKR", "₨", 2},
                {"BDT", "৳", 2},    {"LKR", "Rs", 2},  {"NPR", "Rs", 2},  {"KWD", "د.ك", 3},  {"BHD", ".د.ب", 3},
                {"OMR", "ر.ع.", 3}, {"QAR", "ر.ق", 2}, {"ISK", "kr", 0},  {"HRK", "kn", 2},   {"RSD", "дин.", 2},
                {"GEL", "₾", 2},    {"AMD", "֏", 2},   {"AZN", "₼", 2},   {"UZS", "so'm", 2}, {"MNT", "₮", 2},
                {"KHR", "៛", 2},    {"LAK", "₭", 2},   {"MMK", "K", 2},   {"BND", "B$", 2},
        };

        constexpr name_entry LANGUAGE_NAMES[] = {
                {"af", "Afrikaans", "Afrikaans"},
                {"am", "አማርኛ", "Amharic"},
                {"ar", "العربية", "Arabic"},
                {"az", "azərbaycan", "Azerbaijani"},
                {"be", "беларуская", "Belarusian"},
                {"bg", "български", "Bulgarian"},
                {"bn", "বাংলা", "Bengali"},
                {"ca", "català", "Catalan"},
                {"cs", "čeština", "Czech"},
                {"da", "dansk", "Danish"},
                {"de", "Deutsch", "German"},
                {"el", "Ελληνικά", "Greek"},
                {"en", "English", "English"},
                {"es", "español", "Spanish"},
                {"et", "eesti", "Estonian"},
                {"eu", "euskara", "Basque"},
                {"fa", "فارسی", "Persian"},
                {"fi", "suomi", "Finnish"},
                {"fil", "Filipino", "Filipino"},
                {"fr", "français", "French"},
                {"gl", "galego", "Galician"},
                {"gu", "ગુજરાતી", "Gujarati"},
                {"he", "עברית", "Hebrew"},
                {"hi", "हिन्दी", "Hindi"},
                {"hr", "hrvatski", "Croatian"},
                {"hu", "magyar", "Hungarian"},
                {"hy", "հայերեն", "Armenian"},
                {"id", "Bahasa Indonesia", "Indonesian"},
                {"is", "íslenska", "Icelandic"},
                {"it", "italiano", "Italian"},
                {"ja", "日本語", "Japanese"},
                {"ka", "ქართული", "Georgian"},
                {"kk", "қазақ тілі", "Kazakh"},
                {"km", "ខ្មែរ", "Khmer"},
                {"kn", "ಕನ್ನಡ", "Kannada"},
                {"ko", "한국어", "Korean"},
                {"ky", "кыргызча", "Kyrgyz"},
                {"lo", "ລາວ", "Lao"},
                {"lt", "lietuvių", "Lithuanian"},
                {"lv", "latviešu", "Latvian"},
                {"mk", "македонски", "Macedonian"},
                {"ml", "മലയാളം", "Malayalam"},
                {"mn", "монгол", "Mongolian"},
                {"mr", "मराठी", "Marathi"},
                {"ms", "Bahasa Melayu", "Malay"},
                {"my", "မြန်မာ", "Burmese"},
                {"nb", "norsk bokmål", "Norwegian Bokmål"},
                {"ne", "नेपाली", "Nepali"},
                {"nl", "Nederlands", "Dutch"},
                {"nn", "norsk nynorsk", "Norwegian Nynorsk"},
                {"pa", "ਪੰਜਾਬੀ", "Punjabi"},
                {"pl", "polski", "Polish"},
                {"ps", "پښتو", "Pashto"},
                {"pt", "português", "Portuguese"},
                {"ro", "română", "Romanian"},
                {"ru", "русский", "Russian"},
                {"si", "සිංහල", "Sinhala"},
                {"sk", "slovenčina", "Slovak"},
                {"sl", "slovenščina", "Slovenian"},
                {"sq", "shqip", "Albanian"},
                {"sr", "српски", "Serbian"},
                {"sv", "svenska", "Swedish"},
                {"sw", "Kiswahili", "Swahili"},
                {"ta", "தமிழ்", "Tamil"},
                {"te", "తెలుగు", "Telugu"},
                {"th", "ไทย", "Thai"},
                {"tr", "Türkçe", "Turkish"},
                {"uk", "українська", "Ukrainian"},
                {"ur", "اردو", "Urdu"},
                {"uz", "o'zbek", "Uzbek"},
                {"vi", "Tiếng Việt", "Vietnamese"},
                {"zh", "中文", "Chinese"},
                {"zu", "isiZulu", "Zulu"},
        };

        constexpr name_entry COUNTRY_NAMES[] = {
                {"AE", "الإمارات العربية المتحدة", "United Arab Emirates"},
                {"AR", "Argentina", "Argentina"},
                {"AT", "Österreich", "Austria"},
                {"AU", "Australia", "Australia"},
                {"BD", "বাংলাদেশ", "Bangladesh"},
                {"BE", "België", "Belgium"},
                {"BG", "България", "Bulgaria"},
                {"BR", "Brasil", "Brazil"},
                {"CA", "Canada", "Canada"},
                {"CH", "Schweiz", "Switzerland"},
                {"CL", "Chile", "Chile"},
                {"CN", "中国", "China"},
                {"CO", "Colombia", "Colombia"},
                {"CZ", "Česko", "Czechia"},
                {"DE", "Deutschland", "Germany"},
                {"DK", "Danmark", "Denmark"},
                {"DZ", "الجزائر", "Algeria"},
                {"EE", "Eesti", "Estonia"},
                {"EG", "مصر", "Egypt"},
                {"ES", "España", "Spain"},
                {"ET", "ኢትዮጵያ", "Ethiopia"},
                {"FI", "Suomi", "Finland"},
                {"FR", "France", "France"},
                {"GB", "United Kingdom", "United Kingdom"},
                {"GE", "საქართველო", "Georgia"},
                {"GR", "Ελλάδα", "Greece"},
                {"HK", "中國香港", "Hong Kong SAR China"},
                {"HR", "Hrvatska", "Croatia"},
                {"HU", "Magyarország", "Hungary"},
                {"ID", "Indonesia", "Indonesia"},
                {"IE", "Ireland", "Ireland"},
                {"IL", "ישראל", "Israel"},
                {"IN", "भारत", "India"},
                {"IQ", "العراق", "Iraq"},
                {"IR", "ایران", "Iran"},
                {"IS", "Ísland", "Iceland"},
                {"IT", "Italia", "Italy"},
                {"JP", "日本", "Japan"},
                {"KE", "Kenya", "Kenya"},
                {"KH", "កម្ពុជា", "Cambodia"},
                {"KR", "대한민국", "South Korea"},
                {"KZ", "Қазақстан", "Kazakhstan"},
                {"LK", "ශ්‍රී ලංකාව", "Sri Lanka"},
                {"LT", "Lietuva", "Lithuania"},
                {"LU", "Lëtzebuerg", "Luxembourg"},
                {"LV", "Latvija", "Latvia"},
                {"MA", "المغرب", "Morocco"},
                {"MM", "မြန်မာ", "Myanmar"},
                {"MN", "Монгол улс", "Mongolia"},
                {"MX", "México", "Mexico"},
                {"MY", "Malaysia", "Malaysia"},
                {"NG", "Nigeria", "Nigeria"},
                {"NL", "Nederland", "Netherlands"},
                {"NO", "Norge", "Norway"},
                {"NP", "नेपाल", "Nepal"},
                {"NZ", "New Zealand", "New Zealand"},
                {"PE", "Perú", "Peru"},
                {"PH", "Pilipinas", "Philippines"},
                {"PK", "پاکستان", "Pakistan"},
                {"PL", "Polska", "Poland"},
                {"PT", "Portugal", "Portugal"},
                {"RO", "România", "Romania"},
                {"RS", "Србија", "Serbia"},
                {"RU", "Россия", "Russia"},
                {"SA", "المملكة العربية السعودية", "Saudi Arabia"},
                {"SE", "Sverige", "Sweden"},
                {"SG", "Singapore", "Singapore"},
                {"SI", "Slovenija", "Slovenia"},
                {"SK", "Slovensko", "Slovakia"},
                {"TH", "ไทย", "Thailand"},
                {"TR", "Türkiye", "Türkiye"},
                {"TW", "台灣", "Taiwan"},
                {"UA", "Україна", "Ukraine"},
                {"US", "United States", "United States"},
                {"UZ", "O'zbekiston", "Uzbekistan"},
                {"VN", "Việt Nam", "Vietnam"},
                {"ZA", "South Africa", "South Africa"},
        };

        struct home_currency_entry {
            string_view country;
            string_view currency;
        };

        constexpr home_currency_entry HOME_CURRENCIES[] = {
                {"AE", "AED"}, {"AR", "ARS"}, {"AT", "EUR"}, {"AU", "AUD"}, {"BD", "BDT"}, {"BE", "EUR"}, {"BG", "BGN"},
                {"BR", "BRL"}, {"CA", "CAD"}, {"CH", "CHF"}, {"CL", "CLP"}, {"CN", "CNY"}, {"CO", "COP"}, {"CZ", "CZK"},
                {"DE", "EUR"}, {"DK", "DKK"}, {"DZ", "DZD"}, {"EE", "EUR"}, {"EG", "EGP"}, {"ES", "EUR"}, {"ET", "ETB"},
                {"FI", "EUR"}, {"FR", "EUR"}, {"GB", "GBP"}, {"GE", "GEL"}, {"GR", "EUR"}, {"HK", "HKD"}, {"HR", "EUR"},
                {"HU", "HUF"}, {"ID", "IDR"}, {"IE", "EUR"}, {"IL", "ILS"}, {"IN", "INR"}, {"IQ", "IQD"}, {"IR", "IRR"},
                {"IS", "ISK"}, {"IT", "EUR"}, {"JP", "JPY"}, {"KE", "KES"}, {"KH", "KHR"}, {"KR", "KRW"}, {"KZ", "KZT"},
                {"LK", "LKR"}, {"LT", "EUR"}, {"LU", "EUR"}, {"LV", "EUR"}, {"MA", "MAD"}, {"MM", "MMK"}, {"MN", "MNT"},
                {"MX", "MXN"}, {"MY", "MYR"}, {"NG", "NGN"}, {"NL", "EUR"}, {"NO", "NOK"}, {"NP", "NPR"}, {"NZ", "NZD"},
                {"PE", "PEN"}, {"PH", "PHP"}, {"PK", "PKR"}, {"PL", "PLN"}, {"PT", "EUR"}, {"RO", "RON"}, {"RS", "RSD"},
                {"RU", "RUB"}, {"SA", "SAR"}, {"SE", "SEK"}, {"SG", "SGD"}, {"SI", "EUR"}, {"SK", "EUR"}, {"TH", "THB"},
                {"TR", "TRY"}, {"TW", "TWD"}, {"UA", "UAH"}, {"US", "USD"}, {"UZ", "UZS"}, {"VN", "VND"}, {"ZA", "ZAR"},
        };

        struct percent_sign_entry {
            string_view language;
            string_view percent_sign;
        };

        constexpr percent_sign_entry PERCENT_SIGNS[] = {
                {"ar", "\xD9\xAA"}, {"fa", "\xD9\xAA"},  {"ur", "\xD9\xAA"}, {"ps", "\xD9\xAA"},
                {"sd", "\xD9\xAA"}, {"ckb", "\xD9\xAA"}, {"dv", "\xD9\xAA"},
        };

        struct display_bracket_entry {
            string_view language;
            string_view before;
            string_view after;
        };

        constexpr display_bracket_entry DISPLAY_BRACKETS[] = {
                {"zh", "\xEF\xBC\x88", "\xEF\xBC\x89"},
                {"ja", " (", ")"},
                {"ko", "(", ")"},
        };

        constexpr string_view RTL_LANGUAGES[] = {"ar",  "arc", "ckb", "dv", "fa",  "he", "ks", "ku",
                                                 "nqo", "prs", "ps",  "sd", "syr", "ug", "ur", "yi"};

        constexpr string_view US_MEASUREMENT_COUNTRIES[] = {"US", "LR", "MM"};
        constexpr string_view UK_MEASUREMENT_COUNTRIES[] = {"GB"};

        constexpr string_view SUNDAY_FIRST_COUNTRIES[] = {
                "AG", "AS", "BD", "BR", "BS", "BT", "BW", "BZ", "CA", "CO", "DM", "DO", "ET", "GT",
                "GU", "HK", "HN", "ID", "IL", "IN", "JM", "JP", "KE", "KH", "KR", "LA", "MH", "MM",
                "MO", "MT", "MX", "MZ", "NI", "NP", "PA", "PE", "PH", "PK", "PR", "PY", "SA", "SG",
                "SV", "TH", "TT", "TW", "UM", "US", "VE", "VI", "WS", "ZA", "ZW"};

        constexpr string_view SATURDAY_FIRST_COUNTRIES[] = {"AE", "AF", "BH", "DZ", "EG", "IQ", "IR", "JO",
                                                            "KW", "LY", "OM", "QA", "SD", "SY", "YE"};

        constexpr string_view ISO_639_1_LANGUAGES[] = {
                "aa", "ab", "ae", "af", "ak", "am", "an", "ar", "as", "av", "ay", "az", "ba", "be", "bg", "bh", "bi",
                "bm", "bn", "bo", "br", "bs", "ca", "ce", "ch", "co", "cr", "cs", "cu", "cv", "cy", "da", "de", "dv",
                "dz", "ee", "el", "en", "eo", "es", "et", "eu", "fa", "ff", "fi", "fj", "fo", "fr", "fy", "ga", "gd",
                "gl", "gn", "gu", "gv", "ha", "he", "hi", "ho", "hr", "ht", "hu", "hy", "hz", "ia", "id", "ie", "ig",
                "ii", "ik", "io", "is", "it", "iu", "ja", "jv", "ka", "kg", "ki", "kj", "kk", "kl", "km", "kn", "ko",
                "kr", "ks", "ku", "kv", "kw", "ky", "la", "lb", "lg", "li", "ln", "lo", "lt", "lu", "lv", "mg", "mh",
                "mi", "mk", "ml", "mn", "mr", "ms", "mt", "my", "na", "nb", "nd", "ne", "ng", "nl", "nn", "no", "nr",
                "nv", "ny", "oc", "oj", "om", "or", "os", "pa", "pi", "pl", "ps", "pt", "qu", "rm", "rn", "ro", "ru",
                "rw", "sa", "sc", "sd", "se", "sg", "si", "sk", "sl", "sm", "sn", "so", "sq", "sr", "ss", "st", "su",
                "sv", "sw", "ta", "te", "tg", "th", "ti", "tk", "tl", "tn", "to", "tr", "ts", "tt", "tw", "ty", "ug",
                "uk", "ur", "uz", "ve", "vi", "vo", "wa", "wo", "xh", "yi", "yo", "za", "zh", "zu"};

        constexpr string_view AVAILABLE_LOCALES[] = {
                "af-ZA", "am-ET", "ar-AE", "ar-DZ",       "ar-EG", "ar-IQ", "ar-MA",  "ar-SA", "az-AZ", "be-BY",
                "bg-BG", "bn-BD", "bn-IN", "bs-BA",       "ca-ES", "cs-CZ", "cy-GB",  "da-DK", "de-AT", "de-CH",
                "de-DE", "de-LU", "el-GR", "en-AU",       "en-CA", "en-GB", "en-HK",  "en-IE", "en-IN", "en-NZ",
                "en-PH", "en-SG", "en-US", "en-US-POSIX", "en-ZA", "es-AR", "es-CL",  "es-CO", "es-ES", "es-MX",
                "es-PE", "es-US", "et-EE", "eu-ES",       "fa-IR", "fi-FI", "fil-PH", "fr-BE", "fr-CA", "fr-CH",
                "fr-FR", "fr-LU", "ga-IE", "gl-ES",       "gu-IN", "he-IL", "hi-IN",  "hr-HR", "hu-HU", "hy-AM",
                "id-ID", "is-IS", "it-CH", "it-IT",       "ja-JP", "ka-GE", "kk-KZ",  "km-KH", "kn-IN", "ko-KR",
                "ky-KG", "lo-LA", "lt-LT", "lv-LV",       "mk-MK", "ml-IN", "mn-MN",  "mr-IN", "ms-MY", "mt-MT",
                "my-MM", "nb-NO", "ne-NP", "nl-BE",       "nl-NL", "nn-NO", "pa-IN",  "pl-PL", "ps-AF", "pt-BR",
                "pt-PT", "ro-RO", "ru-RU", "si-LK",       "sk-SK", "sl-SI", "sq-AL",  "sr-RS", "sv-SE", "sw-KE",
                "ta-IN", "te-IN", "th-TH", "tr-TR",       "uk-UA", "ur-PK", "uz-UZ",  "vi-VN", "zh-CN", "zh-HK",
                "zh-MO", "zh-SG", "zh-TW", "zu-ZA"};

        constexpr string_view ISO_COUNTRIES[] = {
                "AD", "AE", "AF", "AG", "AI", "AL", "AM", "AO", "AQ", "AR", "AS", "AT", "AU", "AW", "AX", "AZ", "BA",
                "BB", "BD", "BE", "BF", "BG", "BH", "BI", "BJ", "BL", "BM", "BN", "BO", "BQ", "BR", "BS", "BT", "BV",
                "BW", "BY", "BZ", "CA", "CC", "CD", "CF", "CG", "CH", "CI", "CK", "CL", "CM", "CN", "CO", "CQ", "CR",
                "CU", "CV", "CW", "CX", "CY", "CZ", "DE", "DG", "DJ", "DK", "DM", "DO", "DZ", "EA", "EC", "EE", "EG",
                "EH", "ER", "ES", "ET", "FI", "FJ", "FK", "FM", "FO", "FR", "GA", "GB", "GD", "GE", "GF", "GG", "GH",
                "GI", "GL", "GM", "GN", "GP", "GQ", "GR", "GS", "GT", "GU", "GW", "GY", "HK", "HM", "HN", "HR", "HT",
                "HU", "IC", "ID", "IE", "IL", "IM", "IN", "IO", "IQ", "IR", "IS", "IT", "JE", "JM", "JO", "JP", "KE",
                "KG", "KH", "KI", "KM", "KN", "KP", "KR", "KW", "KY", "KZ", "LA", "LB", "LC", "LI", "LK", "LR", "LS",
                "LT", "LU", "LV", "LY", "MA", "MC", "MD", "ME", "MF", "MG", "MH", "MK", "ML", "MM", "MN", "MO", "MP",
                "MQ", "MR", "MS", "MT", "MU", "MV", "MW", "MX", "MY", "MZ", "NA", "NC", "NE", "NF", "NG", "NI", "NL",
                "NO", "NP", "NR", "NU", "NZ", "OM", "PA", "PE", "PF", "PG", "PH", "PK", "PL", "PM", "PN", "PR", "PS",
                "PT", "PW", "PY", "QA", "RE", "RO", "RS", "RU", "RW", "SA", "SB", "SC", "SD", "SE", "SG", "SH", "SI",
                "SJ", "SK", "SL", "SM", "SN", "SO", "SR", "SS", "ST", "SV", "SX", "SY", "SZ", "TC", "TD", "TF", "TG",
                "TH", "TJ", "TK", "TL", "TM", "TN", "TO", "TR", "TT", "TV", "TW", "TZ", "UA", "UG", "UM", "US", "UY",
                "UZ", "VA", "VC", "VE", "VG", "VI", "VN", "VU", "WF", "WS", "XK", "YE", "YT", "ZA", "ZM", "ZW"};
    } // namespace


    const number_style& number_style_of(string_view language) noexcept {
        for (const auto& entry: NUMBER_STYLES) {
            if (same_code(entry.language, language)) {
                return entry.style;
            }
        }
        return DEFAULT_NUMBER_STYLE;
    }

    const calendar_style& calendar_style_of(string_view language, string_view country) noexcept {
        if (country != nullptr && country[0] != '\0') {
            for (const auto& entry: REGION_STYLES) {
                if (same_code(entry.language, language) && same_code(entry.country, country)) {
                    return *entry.style;
                }
            }
        }
        for (const auto& entry: CALENDAR_STYLES) {
            if (same_code(entry.language, language)) {
                return entry;
            }
        }
        return CALENDAR_STYLES[0];
    }

    const currency_entry* currency_of(string_view iso_4217) noexcept {
        for (const auto& entry: CURRENCIES) {
            if (same_code(entry.code, iso_4217)) {
                return &entry;
            }
        }
        return nullptr;
    }

    const name_entry* language_name_of(string_view language) noexcept {
        for (const auto& entry: LANGUAGE_NAMES) {
            if (same_code(entry.code, language)) {
                return &entry;
            }
        }
        return nullptr;
    }

    const name_entry* country_name_of(string_view country) noexcept {
        for (const auto& entry: COUNTRY_NAMES) {
            if (same_code(entry.code, country)) {
                return &entry;
            }
        }
        return nullptr;
    }

    string_view home_currency_of(string_view country) noexcept {
        for (const auto& entry: HOME_CURRENCIES) {
            if (same_code(entry.country, country)) {
                return entry.currency;
            }
        }
        return "USD";
    }

    string_view percent_sign_of(string_view language) noexcept {
        for (const auto& entry: PERCENT_SIGNS) {
            if (same_code(entry.language, language)) {
                return entry.percent_sign;
            }
        }
        return "%";
    }

    void display_name_brackets(string_view language, string_view& before, string_view& after) noexcept {
        for (const auto& entry: DISPLAY_BRACKETS) {
            if (same_code(entry.language, language)) {
                before = entry.before;
                after = entry.after;
                return;
            }
        }
        before = " (";
        after = ")";
    }

    bool is_right_to_left(string_view language) noexcept {
        for (const string_view code: RTL_LANGUAGES) {
            if (same_code(code, language)) {
                return true;
            }
        }
        return false;
    }

    uint8_t measurement_system_of(string_view country) noexcept {
        if (in_code_list(country, US_MEASUREMENT_COUNTRIES,
                         sizeof(US_MEASUREMENT_COUNTRIES) / sizeof(US_MEASUREMENT_COUNTRIES[0]))) {
            return 1;
        }
        if (in_code_list(country, UK_MEASUREMENT_COUNTRIES,
                         sizeof(UK_MEASUREMENT_COUNTRIES) / sizeof(UK_MEASUREMENT_COUNTRIES[0]))) {
            return 2;
        }
        return 0;
    }

    uint8_t first_day_of_week_of(string_view country) noexcept {
        if (in_code_list(country, SUNDAY_FIRST_COUNTRIES,
                         sizeof(SUNDAY_FIRST_COUNTRIES) / sizeof(SUNDAY_FIRST_COUNTRIES[0]))) {
            return 1;
        }
        if (in_code_list(country, SATURDAY_FIRST_COUNTRIES,
                         sizeof(SATURDAY_FIRST_COUNTRIES) / sizeof(SATURDAY_FIRST_COUNTRIES[0]))) {
            return 7;
        }
        return 2;
    }

    bool is_known_language(string_view language) noexcept {
        if (language == nullptr || language[0] == '\0') {
            return false;
        }
        return in_code_list(language, ISO_639_1_LANGUAGES,
                            sizeof(ISO_639_1_LANGUAGES) / sizeof(ISO_639_1_LANGUAGES[0])) ||
               language_name_of(language) != nullptr;
    }

    array<string_view, 114> available_locales() noexcept { return AVAILABLE_LOCALES; }

    array<string_view, 254> iso_countries() noexcept { return ISO_COUNTRIES; }
} // namespace locale_data

NEFORCE_END_NAMESPACE__
#endif // !NEFORCE_SUPPORT_ICU
