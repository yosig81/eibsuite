add_library(GSM SHARED
    src/gsm_at.cc
    src/gsm_cb.cc
    src/gsm_error.cc
    src/gsm_event.cc
    src/gsm_me_ta.cc
    src/gsm_nls.cc
    src/gsm_parser.cc
    src/gsm_phonebook.cc
    src/gsm_sms.cc
    src/gsm_sms_codec.cc
    src/gsm_sms_store.cc
    src/gsm_sorted_phonebook.cc
    src/gsm_sorted_phonebook_base.cc
    src/gsm_sorted_sms_store.cc
    src/gsm_unix_serial.cc
    src/gsm_util.cc
)

target_include_directories(GSM PUBLIC include)
target_compile_definitions(GSM PRIVATE HAVE_CONFIG_H)
target_link_libraries(GSM PUBLIC jtc)
