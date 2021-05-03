#include "DBHelper.h"

DBHelper::DBHelper(const std::string &sql_query)
    : result(pg_query_scan(sql_query))
    , scan_result(pg_query__scan_result__unpack(NULL, result.pbuf.len, (void *) result.pbuf.data)) {
}

DBHelper::~DBHelper(PgQueryScanResult result, PgQuery__ScanResult *scan_result) {
    pg_query__scan_result__free_unpacked(scan_result, nullptr);
    pg_query_free_scan_result(result);
}

/* Temp func for inform */
void DBHelper::printData() {
    printf("version: %d, tokens: %ld, size: %d\n", scan_result->version, scan_result->n_tokens, result.pbuf.len);

    for (size_t j = 0; j < scan_result->n_tokens; ++j) {
        scan_token = scan_result->tokens[j];
        token_kind = protobuf_c_enum_descriptor_get_value(&pg_query__token__descriptor, scan_token->token);
        keyword_kind = protobuf_c_enum_descriptor_get_value(&pg_query__keyword_kind__descriptor, scan_token->keyword_kind);

        printf("  \"%.*s\" = [ %d, %d, %s, %s ]\n", scan_token->end - scan_token->start, &(input[scan_token->start]), scan_token->start, scan_token->end, token_kind->name, keyword_kind->name);
    }
}