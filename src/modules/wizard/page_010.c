/* Language chooser */
#include "e_wizard.h"

typedef struct _E_Intl_Pair E_Intl_Pair;

static int _basic_lang_list_sort(const void *data1, const void *data2);
EINTERN void _lang_default_pair_set(const char *def_lang);

struct _E_Intl_Pair
{
   const char *locale_key;
   const char *locale_icon;
   const char *locale_translation;
};

const E_Intl_Pair basic_language_predefined_pairs[] =
{
   {"ar_AE.UTF-8", "ara_flag.png", "العربية"},
   {"bg_BG.UTF-8", "bg_flag.png", "Български"},
   {"ca_ES.UTF-8", "cat_flag.png", "Català"},
   {"cs_CZ.UTF-8", "cz_flag.png", "Čeština"},
   {"da_DK.UTF-8", "dk_flag.png", "Dansk"},
   {"de_DE.UTF-8", "de_flag.png", "Deutsch"},
   {"en_US.UTF-8", "us_flag.png", "English"},
   {"en_GB.UTF-8", "gb_flag.png", "British English"},
   {"el_GR.UTF-8", "gr_flag.png", "Ελληνικά"},
   {"eo.UTF-8", "epo_flag.png", "Esperanto"},
   {"es_AR.UTF-8", "ar_flag.png", "Español"},
   {"et_ET.UTF-8", "ee_flag.png", "Eesti keel"},
   {"fi_FI.UTF-8", "fi_flag.png", "Suomi"},
   {"fo_FO.UTF-8", "fo_flag.png", "Føroyskt"},
   {"fr_CH.UTF-8", "ch_flag.png", "Français (Suisse)"},
   {"fr_FR.UTF-8", "fr_flag.png", "Français"},
   {"he_HE.UTF-8", "il_flag.png", "עברית"},
   {"hr_HR.UTF-8", "hr_flag.png", "Hrvatski"},
   {"hu_HU.UTF-8", "hu_flag.png", "Magyar"},
   {"it_IT.UTF-8", "it_flag.png", "Italiano"},
   {"ja_JP.UTF-8", "jp_flag.png", "日本語"},
   {"km_KM.UTF-8", "kh_flag.png", "ភាសាខ្មែរ"},
   {"ko_KR.UTF-8", "kr_flag.png", "한국어"},
   {"ku.UTF-8", "ku_flag.png", "یدروك"},
   {"lt_LT.UTF-8", "lt_flag.png", "Lietuvių kalba"},
   {"ms_MY.UTF-8", "my_flag.png", "Bahasa Melayu"},
   {"nb_NO.UTF-8", "no_flag.png", "Norsk Bokmål"},
   {"nl_NL.UTF-8", "nl_flag.png", "Nederlands"},
   {"pl_PL.UTF-8", "pl_flag.png", "Polski"},
   {"pt_BR.UTF-8", "br_flag.png", "Português"},
   {"ru_RU.UTF-8", "ru_flag.png", "Русский"},
   {"sk_SK.UTF-8", "sk_flag.png", "Slovenčina"},
   {"sl_SI.UTF-8", "si_flag.png", "Slovenščina"},
   {"sv_SE.UTF-8", "se_flag.png", "Svenska"},
   {"tr_TR.UTF-8", "tr_flag.png", "Türkçe"},
   {"uk_UK.UTF-8", "ua_flag.png", "Українська мова"},
   {"zh_CN.UTF-8", "cn_flag.png", "中文 (繁体)"},
   {"zh_TW.UTF-8", "tw_flag.png", "中文 (繁體)"},
   { NULL, NULL, NULL }
};

const E_Intl_Pair language_predefined_pairs[] = {
   {"aa", NULL, "Qafár af"},
   {"af", NULL, "Afrikaans"},
   {"ak", NULL, "Akan"},
   {"am", NULL, "አማርኛ"},
   {"an", NULL, "Aragonés"},
   {"ar", NULL, "ةيبرعلا"},
   {"as", NULL, "অসমীয়া"},
   {"az", NULL, "Azərbaycan dili"},
   {"be", NULL, "Беларуская мова"},
   {"bg", NULL, "Български"},
   {"bn", NULL, "বাংলা"},
   {"br", NULL, "Brezhoneg"},
   {"bs", NULL, "Bosanski"},
   {"byn", NULL, "ብሊና"},
   {"ca", NULL, "Català"},
   {"cch", NULL, "Atsam"},
   {"cs", NULL, "Čeština"},
   {"cy", NULL, "Cymraeg"},
   {"da", NULL, "Dansk"},
   {"de", NULL, "Deutsch"},
   {"dv", NULL, "ދިވެހި"},
   {"dz", NULL, "Dzongkha"},
   {"ee", NULL, "Eʋegbe"},
   {"el", NULL, "Ελληνικά"},
   {"en", NULL, "English"},
   {"eo", NULL, "Esperanto"},
   {"es", NULL, "Español"},
   {"et", NULL, "Eesti keel"},
   {"eu", NULL, "Euskara"},
   {"fa", NULL, "یسراف"},
   {"fi", NULL, "Suomi"},
   {"fo", NULL, "Føroyskt"},
   {"fr", NULL, "Français"},
   {"fur", NULL, "Furlan"},
   {"ga", NULL, "Gaeilge"},
   {"gaa", NULL, "Gã"},
   {"gez", NULL, "ግዕዝ"},
   {"gl", NULL, "Galego"},
   {"gu", NULL, "Gujarati"},
   {"gv", NULL, "Yn Ghaelg"},
   {"ha", NULL, "Hausa"},
   {"haw", NULL, "ʻŌlelo Hawaiʻi"},
   {"he", NULL, "עברית"},
   {"hi", NULL, "Hindi"},
   {"hr", NULL, "Hrvatski"},
   {"hu", NULL, "Magyar"},
   {"hy", NULL, "Հայերեն"},
   {"ia", NULL, "Interlingua"},
   {"id", NULL, "Indonesian"},
   {"ig", NULL, "Igbo"},
   {"is", NULL, "Íslenska"},
   {"it", NULL, "Italiano"},
   {"iu", NULL, "ᐃᓄᒃᑎᑐᑦ"},
   {"iw", NULL, "עברית"},
   {"ja", NULL, "日本語"},
   {"ka", NULL, "ქართული"},
   {"kaj", NULL, "Jju"},
   {"kam", NULL, "Kikamba"},
   {"kcg", NULL, "Tyap"},
   {"kfo", NULL, "Koro"},
   {"kk", NULL, "Qazaq"},
   {"kl", NULL, "Kalaallisut"},
   {"km", NULL, "ភាសាខ្មែរ"},
   {"kn", NULL, "ಕನ್ನಡ"},
   {"ko", NULL, "한국어"},
   {"kok", NULL, "Konkani"},
   {"ku", NULL, "یدروك"},
   {"kw", NULL, "Kernowek"},
   {"ky", NULL, "Кыргыз тили"},
   {"ln", NULL, "Lingála"},
   {"lo", NULL, "ພາສາລາວ"},
   {"lt", NULL, "Lietuvių kalba"},
   {"lv", NULL, "Latviešu"},
   {"mi", NULL, "Te Reo Māori"},
   {"mk", NULL, "Македонски"},
   {"ml", NULL, "മലയാളം"},
   {"mn", NULL, "Монгол"},
   {"mr", NULL, "मराठी"},
   {"ms", NULL, "Bahasa Melayu"},
   {"mt", NULL, "Malti"},
   {"nb", NULL, "Norsk Bokmål"},
   {"ne", NULL, "नेपाली"},
   {"nl", NULL, "Nederlands"},
   {"nn", NULL, "Norsk Nynorsk"},
   {"no", NULL, "Norsk"},
   {"nr", NULL, "isiNdebele"},
   {"nso", NULL, "Sesotho sa Leboa"},
   {"ny", NULL, "Chicheŵa"},
   {"oc", NULL, "Occitan"},
   {"om", NULL, "Oromo"},
   {"or", NULL, "ଓଡ଼ିଆ"},
   {"pa", NULL, "ਪੰਜਾਬੀ"},
   {"pl", NULL, "Polski"},
   {"ps", NULL, "وتښپ"},
   {"pt", NULL, "Português"},
   {"ro", NULL, "Română"},
   {"ru", NULL, "Русский"},
   {"rw", NULL, "Kinyarwanda"},
   {"sa", NULL, "संस्कृतम्"},
   {"se", NULL, "Davvisápmi"},
   {"sh", NULL, "Srpskohrvatski/Српскохрватски"},
   {"sid", NULL, "Sidámo 'Afó"},
   {"sk", NULL, "Slovenčina"},
   {"sl", NULL, "Slovenščina"},
   {"so", NULL, "af Soomaali"},
   {"sq", NULL, "Shqip"},
   {"sr", NULL, "Српски"},
   {"ss", NULL, "Swati"},
   {"st", NULL, "Southern Sotho"},
   {"sv", NULL, "Svenska"},
   {"sw", NULL, "Swahili"},
   {"syr", NULL, "Syriac"},
   {"ta", NULL, "தமிழ்"},
   {"te", NULL, "తెలుగు"},
   {"tg", NULL, "Тоҷикӣ"},
   {"th", NULL, "ภาษาไทย"},
   {"ti", NULL, "ትግርኛ"},
   {"tig", NULL, "ቲግሬ"},
   {"tl", NULL, "Tagalog"},
   {"tn", NULL, "Setswana"},
   {"tr", NULL, "Türkçe"},
   {"ts", NULL, "Tsonga"},
   {"tt", NULL, "Татарча"},
   {"uk", NULL, "Українська мова"},
   {"ur", NULL, "ودراُ"},
   {"uz", NULL, "O‘zbek"},
   {"ve", NULL, "Venda"},
   {"vi", NULL, "Tiếng Việt"},
   {"wa", NULL, "Walon"},
   {"wal", NULL, "Walamo"},
   {"xh", NULL, "Xhosa"},
   {"yi", NULL, "שידיִי"},
   {"yo", NULL, "èdèe Yorùbá"},
   {"zh", NULL, "汉语/漢語"},
   {"zu", NULL, "Zulu"},
   { NULL, NULL, NULL}
};

const E_Intl_Pair region_predefined_pairs[] = {
   { "AF", "af_flag.png", "Afghanistan"},
   { "AX", NULL, "Åland"},
   { "AL", "al_flag.png", "Shqipëria"},
   { "DZ", NULL, "Algeria"},
   { "AS", NULL, "Amerika Sāmoa"},
   { "AD", "ad_flag.png", "Andorra"},
   { "AO", NULL, "Angola"},
   { "AI", NULL, "Anguilla"},
   { "AQ", NULL, "Antarctica"},
   { "AG", NULL, "Antigua and Barbuda"},
   { "AR", "ar_flag.png", "Argentina"},
   { "AM", "am_flag.png", "Հայաստան"},
   { "AW", NULL, "Aruba"},
   { "AU", "au_flag.png", "Australia"},
   { "AT", "at_flag.png", "Österreich"},
   { "AZ", "az_flag.png", "Azərbaycan"},
   { "BS", NULL, "Bahamas"},
   { "BH", NULL, "Bahrain"},
   { "BD", "bd_flag.png", "বাংলাদেশ"},
   { "BB", NULL, "Barbados"},
   { "BY", "by_flag.png", "Беларусь"},
   { "BE", "be_flag.png", "Belgium"},
   { "BZ", NULL, "Belize"},
   { "BJ", NULL, "Bénin"},
   { "BM", NULL, "Bermuda"},
   { "BT", "bt_flag.png", "Bhutan"},
   { "BO", NULL, "Bolivia"},
   { "BA", "ba_flag.png", "Bosnia and Herzegovina"},
   { "BW", "bw_flag.png", "Botswana"},
   { "BV", NULL, "Bouvetøya"},
   { "BR", "br_flag.png", "Brazil"},
   { "IO", NULL, "British Indian Ocean Territory"},
   { "BN", NULL, "Brunei Darussalam"},
   { "BG", "bg_flag.png", "България"},
   { "BF", NULL, "Burkina Faso"},
   { "BI", NULL, "Burundi"},
   { "KH", "kh_flag.png", "Cambodia"},
   { "CM", "cm_flag.png", "Cameroon"},
   { "CA", "ca_flag.png", "Canada"},
   { "CV", NULL, "Cape Verde"},
   { "KY", NULL, "Cayman Islands"},
   { "CF", NULL, "Central African Republic"},
   { "TD", NULL, "Chad"},
   { "CL", NULL, "Chile"},
   { "CN", "cn_flag.png", "中國"},
   { "CX", NULL, "Christmas Island"},
   { "CC", NULL, "Cocos (keeling) Islands"},
   { "CO", NULL, "Colombia"},
   { "KM", NULL, "Comoros"},
   { "CG", NULL, "Congo"},
   { "CD", "cd_flag.png", "Congo"},
   { "CK", NULL, "Cook Islands"},
   { "CR", NULL, "Costa Rica"},
   { "CI", NULL, "Cote d'Ivoire"},
   { "HR", "hr_flag.png", "Hrvatska"},
   { "CU", NULL, "Cuba"},
   { "CY", NULL, "Cyprus"},
   { "CZ", "cz_flag.png", "Česká republika"},
   { "DK", "dk_flag.png", "Danmark"},
   { "DJ", NULL, "Djibouti"},
   { "DM", NULL, "Dominica"},
   { "DO", NULL, "República Dominicana"},
   { "EC", NULL, "Ecuador"},
   { "EG", "eg_flag.png", "Egypt"},
   { "SV", NULL, "El Salvador"},
   { "GQ", NULL, "Equatorial Guinea"},
   { "ER", NULL, "Eritrea"},
   { "EE", "ee_flag.png", "Eesti"},
   { "ET", "et_flag.png", "Ethiopia"},
   { "FK", NULL, "Falkland Islands (malvinas)"},
   { "FO", "fo_flag.png", "Faroe Islands"},
   { "FJ", NULL, "Fiji"},
   { "FI", "fi_flag.png", "Finland"},
   { "FR", "fr_flag.png", "France"},
   { "GF", NULL, "French Guiana"},
   { "PF", NULL, "French Polynesia"},
   { "TF", NULL, "French Southern Territories"},
   { "GA", NULL, "Gabon"},
   { "GM", NULL, "Gambia"},
   { "GE", "ge_flag.png", "Georgia"},
   { "DE", "de_flag.png", "Deutschland"},
   { "GH", "gh_flag.png", "Ghana"},
   { "GI", NULL, "Gibraltar"},
   { "GR", "gr_flag.png", "Greece"},
   { "GL", NULL, "Greenland"},
   { "GD", NULL, "Grenada"},
   { "GP", NULL, "Guadeloupe"},
   { "GU", NULL, "Guam"},
   { "GT", NULL, "Guatemala"},
   { "GG", NULL, "Guernsey"},
   { "GN", "gn_flag.png", "Guinea"},
   { "GW", NULL, "Guinea-Bissau"},
   { "GY", NULL, "Guyana"},
   { "HT", NULL, "Haiti"},
   { "HM", NULL, "Heard Island and Mcdonald Islands"},
   { "VA", NULL, "Holy See (Vatican City State)"},
   { "HN", NULL, "Honduras"},
   { "HK", NULL, "Hong Kong"},
   { "HU", "hu_flag.png", "Magyarország"},
   { "IS", "is_flag.png", "Iceland"},
   { "IN", "in_flag.png", "India"},
   { "ID", "id_flag.png", "Indonesia"},
   { "IR", "ir_flag.png", "Iran"},
   { "IQ", "iq_flag.png", "Iraq"},
   { "IE", "ie_flag.png", "Éire"},
   { "IM", NULL, "Isle Of Man"},
   { "IL", "il_flag.png", "Israel"},
   { "IT", "it_flag.png", "Italia"},
   { "JM", NULL, "Jamaica"},
   { "JP", "jp_flag.png", "日本"},
   { "JE", NULL, "Jersey"},
   { "JO", NULL, "Jordan"},
   { "KZ", "kz_flag.png", "Kazakhstan"},
   { "KE", "ke_flag.png", "Kenya"},
   { "KI", NULL, "Kiribati"},
   { "KP", NULL, "Korea"},
   { "KR", "kr_flag.png", "Korea"},
   { "KW", NULL, "Kuwait"},
   { "KG", "kg_flag.png", "Kyrgyzstan"},
   { "LA", "la_flag.png", "Lao People's Democratic Republic"},
   { "LV", "lv_flag.png", "Latvija"},
   { "LB", NULL, "Lebanon"},
   { "LS", NULL, "Lesotho"},
   { "LR", NULL, "Liberia"},
   { "LY", NULL, "Libyan Arab Jamahiriya"},
   { "LI", NULL, "Liechtenstein"},
   { "LT", "lt_flag.png", "Lietuva"},
   { "LU", NULL, "Lëtzebuerg"},
   { "MO", NULL, "Macao"},
   { "MK", "mk_flag.png", "Македонија"},
   { "MG", "mg_flag.png", "Madagascar"},
   { "MW", NULL, "Malawi"},
   { "MY", "my_flag.png", "Malaysia"},
   { "MV", "mv_flag.png", "Maldives"},
   { "ML", "ml_flag.png", "Mali"},
   { "MT", "mt_flag.png", "Malta"},
   { "MH", NULL, "Marshall Islands"},
   { "MQ", NULL, "Martinique"},
   { "MR", NULL, "Mauritania"},
   { "MU", NULL, "Mauritius"},
   { "YT", NULL, "Mayotte"},
   { "MX", NULL, "Mexico"},
   { "FM", NULL, "Micronesia"},
   { "MD", NULL, "Moldova"},
   { "MC", NULL, "Monaco"},
   { "MN", "mn_flag.png", "Mongolia"},
   { "ME", NULL, "Montenegro"},
   { "MS", NULL, "Montserrat"},
   { "MA", "ma_flag.png", "Morocco"},
   { "MZ", NULL, "Mozambique"},
   { "MM", "mm_flag.png", "Myanmar"},
   { "NA", NULL, "Namibia"},
   { "NR", NULL, "Nauru"},
   { "NP", "np_flag.png", "Nepal"},
   { "NL", "nl_flag.png", "Nederland"},
   { "AN", NULL, "Netherlands Antilles"},
   { "NC", NULL, "New Caledonia"},
   { "NZ", NULL, "New Zealand"},
   { "NI", NULL, "Nicaragua"},
   { "NE", NULL, "Niger"},
   { "NG", "ng_flag.png", "Nigeria"},
   { "NU", NULL, "Niue"},
   { "NF", NULL, "Norfolk Island"},
   { "MP", NULL, "Northern Mariana Islands"},
   { "NO", "no_flag.png", "Norge"},
   { "OM", NULL, "Oman"},
   { "PK", "pk_flag.png", "Pakistan"},
   { "PW", NULL, "Palau"},
   { "PS", NULL, "Palestinian Territory"},
   { "PA", NULL, "Panama"},
   { "PG", NULL, "Papua New Guinea"},
   { "PY", NULL, "Paraguay"},
   { "PE", NULL, "Peru"},
   { "PH", "ph_flag.png", "Philippines"},
   { "PN", NULL, "Pitcairn"},
   { "PL", "pl_flag.png", "Poland"},
   { "PT", "pt_flag.png", "Portugal"},
   { "PR", NULL, "Puerto Rico"},
   { "QA", NULL, "Qatar"},
   { "RE", NULL, "Reunion"},
   { "RO", "ro_flag.png", "Romania"},
   { "RU", "ru_flag.png", "Russian Federation"},
   { "RW", NULL, "Rwanda"},
   { "SH", NULL, "Saint Helena"},
   { "KN", NULL, "Saint Kitts and Nevis"},
   { "LC", NULL, "Saint Lucia"},
   { "PM", NULL, "Saint Pierre and Miquelon"},
   { "VC", NULL, "Saint Vincent and the Grenadines"},
   { "WS", NULL, "Samoa"},
   { "SM", NULL, "San Marino"},
   { "ST", NULL, "Sao Tome and Principe"},
   { "SA", NULL, "Saudi Arabia"},
   { "SN", "sn_flag.png", "Senegal"},
   { "RS", "rs_flag.png", "Serbia"},
   { "SC", NULL, "Seychelles"},
   { "SL", NULL, "Sierra Leone"},
   { "SG", NULL, "Singapore"},
   { "SK", "sk_flag.png", "Slovakia"},
   { "SI", "si_flag.png", "Slovenia"},
   { "SB", NULL, "Solomon Islands"},
   { "SO", "so_flag.png", "Somalia"},
   { "ZA", "za_flag.png", "South Africa"},
   { "GS", NULL, "South Georgia and the South Sandwich Islands"},
   { "ES", "es_flag.png", "Spain"},
   { "LK", "lk_flag.png", "Sri Lanka"},
   { "SD", NULL, "Sudan"},
   { "SR", NULL, "Suriname"},
   { "SJ", NULL, "Svalbard and Jan Mayen"},
   { "SZ", NULL, "Swaziland"},
   { "SE", "se_flag.png", "Sweden"},
   { "CH", "ch_flag.png", "Switzerland"},
   { "SY", "sy_flag.png", "Syrian Arab Republic"},
   { "TW", "tw_flag.png", "Taiwan"},
   { "TJ", "tj_flag.png", "Tajikistan"},
   { "TZ", "tz_flag.png", "Tanzania"},
   { "TH", "th_flag.png", "Thailand"},
   { "TL", NULL, "Timor-Leste"},
   { "TG", NULL, "Togo"},
   { "TK", NULL, "Tokelau"},
   { "TO", NULL, "Tonga"},
   { "TT", NULL, "Trinidad and Tobago"},
   { "TN", NULL, "Tunisia"},
   { "TR", "tr_flag.png", "Turkey"},
   { "TM", "tm_flag.png", "Turkmenistan"},
   { "TC", NULL, "Turks and Caicos Islands"},
   { "TV", NULL, "Tuvalu"},
   { "UG", NULL, "Uganda"},
   { "UA", "ua_flag.png", "Ukraine"},
   { "AE", NULL, "United Arab Emirates"},
   { "GB", "gb_flag.png", "United Kingdom"},
   { "US", "us_flag.png", "United States"},
   { "UM", NULL, "United States Minor Outlying Islands"},
   { "UY", NULL, "Uruguay"},
   { "UZ", "uz_flag.png", "Uzbekistan"},
   { "VU", NULL, "Vanuatu"},
   { "VE", NULL, "Venezuela"},
   { "VN", "vn_flag.png", "Viet Nam"},
   { "VG", NULL, "Virgin Islands"},
   { "VI", NULL, "Virgin Islands"},
   { "WF", NULL, "Wallis and Futuna"},
   { "EH", NULL, "Western Sahara"},
   { "YE", NULL, "Yemen"},
   { "ZM", NULL, "Zambia"},
   { "ZW", NULL, "Zimbabwe"},
   { NULL, NULL, NULL}
};


static const char *lang = NULL;
static E_Intl_Pair default_pair;
static Eina_List *blang_list = NULL;

static int
_basic_lang_list_sort(const void *data1, const void *data2)
{
   E_Intl_Pair *ln1, *ln2;
   const char *trans1;
   const char *trans2;

   if (!data1) return 1;
   if (!data2) return -1;

   ln1 = (E_Intl_Pair *)data1;
   ln2 = (E_Intl_Pair *)data2;

   if (!ln1->locale_translation) return 1;
   trans1 = ln1->locale_translation;

   if (!ln2->locale_translation) return -1;
   trans2 = ln2->locale_translation;

   return strcmp(trans1, trans2);
}

EAPI int
wizard_page_init(E_Wizard_Page *pg __UNUSED__, Eina_Bool *need_xdg_desktops __UNUSED__, Eina_Bool *need_xdg_icons __UNUSED__)
{
   FILE *output;

#ifdef __OpenBSD__
   output = popen("ls /usr/share/locale", "r");
#else
   output = popen("locale -a", "r");
#endif
   if (output)
     {
        char line[32];

        while (fscanf(output, "%[^\n]\n", line) == 1)
          {
             E_Locale_Parts *locale_parts;

             locale_parts = e_intl_locale_parts_get(line);
             if (locale_parts)
               {
                  char *basic_language = NULL;
                  Eina_Bool found = EINA_FALSE;

                  if (locale_parts->mask & E_INTL_LOC_REGION)
                    basic_language = e_intl_locale_parts_combine(locale_parts,
                      E_INTL_LOC_LANG | E_INTL_LOC_REGION);
                  else if (locale_parts->lang)
                    basic_language = strdup(locale_parts->lang);
                  if (basic_language)
                    {
                       int i = 0;

                       while (basic_language_predefined_pairs[i].locale_key)
                         {
                            /* if basic language is supported by E and System*/
                            if (!strncmp
                                  (basic_language_predefined_pairs[i].locale_key,
                                  basic_language, strlen(basic_language)))
                              {
                                 if (!eina_list_data_find(
                                       blang_list,
                                       &basic_language_predefined_pairs[i]))
                                   {
                                      blang_list = eina_list_append(
                                         blang_list,
                                         &basic_language_predefined_pairs[i]);
                                      found = EINA_TRUE;
                                   }
                                 break;
                              }
                            i++;
                         }
                    }

                  if (!found)
                    {
                       E_Intl_Pair *intl_pair;

                       _lang_default_pair_set(strdup(line));

                       intl_pair = E_NEW(E_Intl_Pair, 1);
                       intl_pair->locale_key = default_pair.locale_key;
                       intl_pair->locale_icon = default_pair.locale_icon;
                       intl_pair->locale_translation = default_pair.locale_translation;

                       blang_list = eina_list_append(blang_list, intl_pair);
                    }

                  E_FREE(basic_language);
                  e_intl_locale_parts_free(locale_parts);
               }
          }
        /* Sort basic languages */
        blang_list = eina_list_sort(blang_list, eina_list_count(blang_list), _basic_lang_list_sort);
        pclose(output);
     }
   return 1;
}
/*
EAPI int
wizard_page_shutdown(E_Wizard_Page *pg __UNUSED__)
{
   // FIXME: free blang_list
   return 1;
}
*/

EINTERN void
_lang_default_pair_set(const char *def_lang)
{
   char *region = NULL;
   char *language = NULL;
   char **split, **locale = NULL;
   int len, i = 0;
   unsigned int size;

   if (!def_lang)
     def_lang = e_intl_language_get();

   if (!def_lang) return;

   len = strlen(def_lang);
   if (len <= 2) return;

   split = eina_str_split_full(def_lang, ".", 0, &size);
   if (size == 1)
     split = eina_str_split_full(def_lang, " ", 0, &size);

   if (size == 1)
     {
        fprintf(stdout, "Error: unable to split string\n");
        free(split[0]);
        free(split);
        return;
     }

   locale = eina_str_split(split[i], "_", 0);
   if ((locale[1]) && (strlen(locale[1]) == 2))
     {
        region = strdup(locale[1]);
        language = strdup(locale[0]);
        free(locale[0]);
        free(locale);
     }

   if (split[0])
     {
        free(split[0]);
        free(split);
     }

   if (region)
     {
        i = 0;
        while (region_predefined_pairs[i].locale_key)
          {
             if (!strcmp(region_predefined_pairs[i].locale_key, region))
               {
                  default_pair = region_predefined_pairs[i];
                  default_pair.locale_key = def_lang;
                  break;
               }
             i++;
          }
        free(region);
     }

   if (language)
     {
        i = 0;
        while (language_predefined_pairs[i].locale_key)
          {
             if (!strcmp(language_predefined_pairs[i].locale_key, language))
               {
                  if (!default_pair.locale_icon)
                    {
                       char buf[256];

                       snprintf(buf, 255, "%s - [ %s ]",
                                language_predefined_pairs[i].locale_translation,
                                default_pair.locale_translation);
                       default_pair.locale_translation = strdup(buf);
                    }
                  else
                    default_pair.locale_translation =
                       language_predefined_pairs[i].locale_translation;
                  break;
               }
             i++;
          }
        free(language);
     }
}

EAPI int
wizard_page_show(E_Wizard_Page *pg)
{
   TS(__FILE__);
   Evas_Object *o, *of, *ob, *ic;
   Eina_List *l;
   int i, sel = -1;
   char buf[PATH_MAX];

   o = e_widget_list_add(pg->evas, 1, 0);
   e_wizard_title_set(_("Language"));
   of = e_widget_framelist_add(pg->evas, _("Select one"), 0);
   ob = e_widget_ilist_add(pg->evas, 32 * e_scale, 32 * e_scale, &lang);
   e_widget_size_min_set(ob, 140 * e_scale, 140 * e_scale);

   e_widget_ilist_freeze(ob);

   e_prefix_data_snprintf(buf, sizeof(buf), "data/flags/%s", "lang-system.png");
   ic = e_util_icon_add(buf, pg->evas);
   e_widget_ilist_append(ob, ic, _("System Default"),
                         NULL, NULL, NULL);
   _lang_default_pair_set(NULL);
   for (i = 1, l = blang_list; l; l = l->next, i++)
     {
        E_Intl_Pair *pair;

        pair = l->data;
        if (pair->locale_icon)
          {
             e_prefix_data_snprintf(buf, sizeof(buf), "data/flags/%s", pair->locale_icon);
             ic = e_util_icon_add(buf, pg->evas);
          }
        else
          ic = NULL;

        if ((pair->locale_key) && e_intl_language_get())
          {
             if (!strcmp(pair->locale_key, e_intl_language_get()))
               {
                  snprintf(buf, sizeof(buf), "System Default - [%s]", pair->locale_translation);
                  e_widget_ilist_nth_label_set(ob, 0, _(buf));
                  e_widget_ilist_nth_icon_set(ob, 0, ic);
                  sel = 0;
               }
            else
              e_widget_ilist_append(ob, ic, _(pair->locale_translation),
                                    NULL, NULL, pair->locale_key);
          }
        else
          e_widget_ilist_append(ob, ic, _(pair->locale_translation),
                                NULL, NULL, pair->locale_key);
     }
   e_widget_ilist_go(ob);
   e_widget_ilist_thaw(ob);
   if (sel >= 0)
     {
        e_widget_ilist_selected_set(ob, sel);
        e_widget_ilist_nth_show(ob, sel, 0);
     }
   else
     {
        if (default_pair.locale_translation)
          snprintf(buf, sizeof(buf), "System Default - [%s]", default_pair.locale_translation);
        else
          snprintf(buf, sizeof(buf), "System Default - [%s]", e_intl_language_get());

        e_widget_ilist_nth_label_set(ob, 0, _(buf));

        if (default_pair.locale_icon)
          {
             e_prefix_data_snprintf(buf, sizeof(buf), "data/flags/%s", default_pair.locale_icon);
             ic = e_util_icon_add(buf, pg->evas);
             e_widget_ilist_nth_icon_set(ob, 0, ic);
          }
        e_widget_ilist_selected_set(ob, 0);
     }

   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   evas_object_show(ob);
   evas_object_show(of);
   e_wizard_page_show(o);
//   pg->data = o;
   return 1; /* 1 == show ui, and wait for user, 0 == just continue */
}

EAPI int
wizard_page_hide(E_Wizard_Page *pg __UNUSED__)
{
//   evas_object_del(pg->data);
/* special - language inits its stuff the moment it goes away */
   eina_stringshare_del(e_config->language);
   e_config->language = eina_stringshare_ref(lang);
   /* TODO:
    * This should be on lang select,
    * so if next page needs xdg we can't press next */

   e_wizard_xdg_desktops_reset();
   e_intl_language_set(e_config->language);
   e_wizard_labels_update();
   return 1;
}

EAPI int
wizard_page_apply(E_Wizard_Page *pg __UNUSED__)
{
   // do this again as we want it to apply to the new profile
   eina_stringshare_del(e_config->language);
   e_config->language = eina_stringshare_ref(lang);
   e_intl_language_set(e_config->language);
   e_wizard_labels_update();
   return 1;
}

