#ifndef __DB_HELPER_H__
#define __DB_HELPER_H__

#include <pg_query.h>
#include <stdio.h>
#include "protobuf/pg_query.pb-c.h"

class DBHelper {
public:
    /* Constructors */
    DBHelper() = delete;
    explicit DBHelper(const std::string &sql_query);

    /* Destructors */
    ~DBHelper(PgQueryScanResult result, PgQuery__ScanResult *scan_result);

    /* Temp funcs */
    void printData();
    

private:
    PgQueryScanResult result;
    PgQuery__ScanResult *scan_result;
    PgQuery__ScanToken *scan_token;
    const ProtobufCEnumValue *token_kind;
    const ProtobufCEnumValue *keyword_kind;
}

#endif  // __DB_HELPER_H__


/*
    typedef struct {
      PgQueryProtobuf pbuf;
      char* stderr_buffer;
      PgQueryError* error;
    } PgQueryScanResult;


    typedef struct _PgQuery__ScanResult PgQuery__ScanResult;
    struct  _PgQuery__ScanResult
    {
      ProtobufCMessage base;
      int32_t version;
      size_t n_tokens;
      PgQuery__ScanToken **tokens;
    };


    typedef struct _PgQuery__ScanToken PgQuery__ScanToken;
    struct  _PgQuery__ScanToken
    {
      ProtobufCMessage base;
      int32_t start;
      int32_t end;
      PgQuery__Token token;
      PgQuery__KeywordKind keyword_kind;
    };


    PgQueryScanResult pg_query_scan(const char* input)
    {
      MemoryContext ctx = NULL;
      PgQueryScanResult result = {0};
      core_yyscan_t yyscanner;
      core_yy_extra_type yyextra;
      core_YYSTYPE yylval;
      YYLTYPE    yylloc;
      PgQuery__ScanResult scan_result = PG_QUERY__SCAN_RESULT__INIT;
      PgQuery__ScanToken **output_tokens;
      size_t token_count = 0;
      size_t i;

      ctx = pg_query_enter_memory_context();

      MemoryContext parse_context = CurrentMemoryContext;

      char stderr_buffer[STDERR_BUFFER_LEN + 1] = {0};
    #ifndef DEBUG
      int stderr_global;
      int stderr_pipe[2];
    #endif

    #ifndef DEBUG
      // Setup pipe for stderr redirection
      if (pipe(stderr_pipe) != 0) {
        PgQueryError* error = malloc(sizeof(PgQueryError));

        error->message = strdup("Failed to open pipe, too many open file descriptors")

        result.error = error;

        return result;
      }

      fcntl(stderr_pipe[0], F_SETFL, fcntl(stderr_pipe[0], F_GETFL) | O_NONBLOCK);

      // Redirect stderr to the pipe
      stderr_global = dup(STDERR_FILENO);
      dup2(stderr_pipe[1], STDERR_FILENO);
      close(stderr_pipe[1]);
    #endif

      PG_TRY();
      {
        // Really this is stupid, we only run twice so we can pre-allocate the output array correctly
        yyscanner = scanner_init(input, &yyextra, &ScanKeywords, ScanKeywordTokens);
        for (;; token_count++)
        {
          if (core_yylex(&yylval, &yylloc, yyscanner) == 0) break;
        }
        scanner_finish(yyscanner);

        output_tokens = malloc(sizeof(PgQuery__ScanToken *) * token_count);

        yyscanner = scanner_init(input, &yyextra, &ScanKeywords, ScanKeywordTokens);

        for (i = 0; ; i++)
        {
          int tok;
          int keyword;

          tok = core_yylex(&yylval, &yylloc, yyscanner);
          if (tok == 0) break;

          output_tokens[i] = malloc(sizeof(PgQuery__ScanToken));
          pg_query__scan_token__init(output_tokens[i]);
          output_tokens[i]->start = yylloc;
          if (tok == SCONST || tok == BCONST || tok == XCONST || tok == IDENT || tok == C_COMMENT) {
            output_tokens[i]->end = yyextra.yyllocend;
          } else {
            output_tokens[i]->end = yylloc + ((struct yyguts_t*) yyscanner)->yyleng_r;
          }
          output_tokens[i]->token = tok;

          switch (tok) {
          #define PG_KEYWORD(a,b,c) case b: output_tokens[i]->keyword_kind = c + 1; break;
          #include "parser/kwlist.h"
          default: output_tokens[i]->keyword_kind = 0;
          }
        }

        scanner_finish(yyscanner);

        scan_result.version = PG_VERSION_NUM;
        scan_result.n_tokens = token_count;
        scan_result.tokens = output_tokens;
        result.pbuf.len = pg_query__scan_result__get_packed_size(&scan_result);
        result.pbuf.data = malloc(result.pbuf.len);
        pg_query__scan_result__pack(&scan_result, (void*) result.pbuf.data);

        for (i = 0; i < token_count; i++) {
          free(output_tokens[i]);
        }
        free(output_tokens);

    #ifndef DEBUG
        // Save stderr for result
        read(stderr_pipe[0], stderr_buffer, STDERR_BUFFER_LEN);
    #endif

        result.stderr_buffer = strdup(stderr_buffer);
      }
      PG_CATCH();
      {
        ErrorData* error_data;
        PgQueryError* error;

        MemoryContextSwitchTo(parse_context);
        error_data = CopyErrorData();

        // Note: This is intentionally malloc so exiting the memory context doesn't free this
        error = malloc(sizeof(PgQueryError));
        error->message   = strdup(error_data->message);
        error->filename  = strdup(error_data->filename);
        error->funcname  = strdup(error_data->funcname);
        error->context   = NULL;
        error->lineno    = error_data->lineno;
        error->cursorpos = error_data->cursorpos;

        result.error = error;
        FlushErrorState();
      }
      PG_END_TRY();

    #ifndef DEBUG
      // Restore stderr, close pipe
      dup2(stderr_global, STDERR_FILENO);
      close(stderr_pipe[0]);
      close(stderr_global);
    #endif

      pg_query_exit_memory_context(ctx);                                

      return result;
    }


    int main() {
      PgQueryScanResult result;
      PgQuery__ScanResult *scan_result;
      PgQuery__ScanToken *scan_token;
      const ProtobufCEnumValue *token_kind;
      const ProtobufCEnumValue *keyword_kind;
      const char *input = "SELECT update AS left FROM between";

      result = pg_query_scan(input);
      scan_result = pg_query__scan_result__unpack(NULL, result.pbuf.len, (void *) result.pbuf.data);

      printf("  version: %d, tokens: %ld, size: %d\n", scan_result->version, scan_result->n_tokens, result.pbuf.len);
      for (size_t j = 0; j < scan_result->n_tokens; j++) {
          scan_token = scan_result->tokens[j];
          token_kind = protobuf_c_enum_descriptor_get_value(&pg_query__token__descriptor, scan_token->token);
          keyword_kind = protobuf_c_enum_descriptor_get_value(&pg_query__keyword_kind__descriptor, scan_token->keyword_kind);
          printf("  \"%.*s\" = [ %d, %d, %s, %s ]\n", scan_token->end - scan_token->start, &(input[scan_token->start]), scan_token->start, scan_token->end, token_kind->name, keyword_kind->name);
      }

      pg_query__scan_result__free_unpacked(scan_result, NULL);
      pg_query_free_scan_result(result);

      return 0;
    }
*/

