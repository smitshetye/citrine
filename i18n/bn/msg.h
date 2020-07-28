#define CTR_MSG_LANG_CODE "bn"
#define CTR_MSG_WELCOME   "Citrine/BN\n"
#define CTR_MSG_COPYRIGHT "Gabor de Mooij ©, Licensed BSD\n"
#define CTR_MSG_USAGE_G   "পর্যাপ্ত যুক্তি নেই। ব্যবহার:: ctr -g file1.h file2.h"
#define CTR_MSG_USAGE_T   "পর্যাপ্ত যুক্তি নেই। ব্যবহার:: ctr -t d.dict program.ctr"
#define CTR_ERR_LEX        " %s  লাইনে: %d ।"
#define CTR_ERR_TOKBUFF    "টোকেন বাফার নিখরচায়। টোকেনগুলি 255 বাইটের বেশি হতে পারে না"
#define CTR_ERR_OOM        "জ্ঞানের বাহিরে"
#define CTR_ERR_SYNTAX     "পার্স ত্রুটি অপ্রত্যাশিত %s  ( %s : %d )"
#define CTR_ERR_LONG       "বার্তাটি খুব দীর্ঘ।"
#define CTR_ERR_EXP_COLON "প্রত্যাশিত [:].\n"
#define CTR_ERR_EXP_MSG "প্রত্যাশিত [বার্তা].\n"
#define CTR_ERR_EXP_PCLS  "প্রত্যাশিত [)].\n"
#define CTR_ERR_EXP_DOT    "লাইনের প্রত্যাশিত সমাপ্তি"
#define CTR_ERR_EXP_KEY    "কী সর্বদা একটি সম্পত্তি নাম অনুসরণ করা উচিত!"
#define CTR_ERR_EXP_VAR    "পয়েন্টিং হ্যান্ড সবসময় চলক অনুসরণ করা উচিত!"
#define CTR_ERR_EXP_RCP    "কোনও বার্তা প্রাপক প্রত্যাশিত।"
#define CTR_ERR_EXP_MSG2   "প্রাপক কোলন অনুসরণ করা যাবে না।"
#define CTR_ERR_INV_LAS    "অবৈধ বাম হাতের অ্যাসাইনমেন্ট।"
#define CTR_ERR_EXP_BLK   "প্রত্যাশিত [কোড]."
#define CTR_ERR_EXP_ARR   "প্রত্যাশিত [তালিকা]."
#define CTR_ERR_EXP_NUM   "প্রত্যাশিত [সংখ্যা]."
#define CTR_ERR_EXP_STR   "প্রত্যাশিত [দড়ি]."
#define CTR_ERR_DIVZERO    "শূন্য দ্বারা বিভাগ"
#define CTR_ERR_BOUNDS     "সীমা ছাড়িয়ে সূচক।"
#define CTR_ERR_REGEX      "নিয়মিত অভিব্যক্তি সংকলন করতে পারেনি।"
#define CTR_ERR_SIPHKEY    "কীটি অবশ্যই 16 বাইট দীর্ঘ হতে হবে।"
#define CTR_SYM_OBJECT    "[Object]"
#define CTR_SYM_BLOCK     "[Block]"
#define CTR_SYM_FILE      "[File (no path)]"
#define CTR_ERR_OPEN       "খুলতে অক্ষম: %s ।"
#define CTR_ERR_DELETE     "মুছতে অক্ষম।"
#define CTR_ERR_FOPENED    "ফাইলটি ইতিমধ্যে খোলা হয়েছে।"
#define CTR_ERR_SEEK       "অনুসন্ধান ব্যর্থ হয়েছে।"
#define CTR_ERR_LOCK       "লক করতে অক্ষম।"
#define CTR_ERR_RET        "অবৈধ রিটার্ন এক্সপ্রেশন।"
#define CTR_ERR_SEND       "প্রকারের রিসিভারকে বার্তা পাঠানো যায় না: %d "
#define CTR_ERR_KEYINX     "কী সূচক ক্লান্ত।"
#define CTR_ERR_ANOMALY    "অসঙ্গতি বার্তা স্তর সনাক্ত।"
#define CTR_ERR_UNCAUGHT   "আনচড ত্রুটি ঘটেছে।"
#define CTR_ERR_NODE       "রানটাইম ত্রুটি। অবৈধ পার্স নোড: %d  %s "
#define CTR_ERR_MISSING    "পার্স নোড হারিয়েছে"
#define CTR_ERR_FOPEN      "ফাইলটি খোলার সময় ত্রুটি।"
#define CTR_ERR_RNUM  "ফিরে আসতে হবে [সংখ্যা].\n"
#define CTR_ERR_RSTR  "ফিরে আসতে হবে [দড়ি].\n"
#define CTR_ERR_RBOOL "ফিরে আসতে হবে [বুলিয়ান].\n"
#define CTR_ERR_NESTING    "অনেক বেশি নেস্টেড কল।"
#define CTR_ERR_KNF        "কী পাওয়া যায় নি:"
#define CTR_ERR_ASSIGN     "অপরিবর্তিত ভেরিয়েবলকে বরাদ্দ করা যায় না:"
#define CTR_ERR_EXEC       "কমান্ড কার্যকর করতে অক্ষম।"
#define CTR_MSG_DSC_FILE  "file"
#define CTR_MSG_DSC_FLDR  "folder"
#define CTR_MSG_DSC_SLNK  "symbolic link"
#define CTR_MSG_DSC_CDEV  "character device"
#define CTR_MSG_DSC_BDEV  "block device"
#define CTR_MSG_DSC_SOCK  "socket"
#define CTR_MSG_DSC_NPIP  "named pipe"
#define CTR_MSG_DSC_OTHR  "other"
#define CTR_MSG_DSC_TYPE  "type"
#define CTR_TERR_LMISMAT  "Language mismatch."
#define CTR_TERR_LONG     "Translation error, message too long."
#define CTR_TERR_DICT     "Error opening dictionary."
#define CTR_TERR_KMISMAT  "Error: key mismatch %s %s on line %d\n"
#define CTR_TERR_ELONG    "Dictionary entry too long."
#define CTR_TERR_AMWORD   "Ambiguous word in dictionary: %s."
#define CTR_TERR_AMTRANS  "Ambiguous translation in dictionary: %s."
#define CTR_TERR_TMISMAT  "Keyword/Binary mismatch:"
#define CTR_TERR_BUFF     "Unable to copy translation to buffer."
#define CTR_TERR_WARN     "Warning: Not translated: "
#define CTR_TERR_TOK      "Token length exceeds maximum buffer size."
#define CTR_TERR_PART      "কীওয়ার্ড বার্তার টোকেনের অংশটি বাফার সীমা অতিক্রম করেছে।"
#define CTR_TERR_COLONS   "Different no. of colons."
#define CTR_MSG_ERROR      "ত্রুটি"
#define CTR_MERR_OOM      "জ্ঞানের বাহিরে  %lu  বাইট বরাদ্দ করতে ব্যর্থ।\n"
#define CTR_MERR_MALLOC   "জ্ঞানের বাহিরে  %lu  বাইট বরাদ্দ করতে ব্যর্থ। (malloc). \n"
#define CTR_MERR_POOL     "Unable to allocate memory pool.\n"
#define CTR_STDDATEFRMT   "%Y-%m-%d %H:%M:%S"
#define CTR_STDTIMEZONE   "UTC"