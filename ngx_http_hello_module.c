#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

/* 模块location配置 */
typedef struct {
    ngx_str_t  hello_string;
    ngx_int_t  hello_count;
}ngx_http_hello_loc_conf_t;

/* 初始化模块方法 */
static ngx_int_t ngx_http_hello_init(ngx_conf_t *cf);

/* 创建模块配置方法 */
static void *ngx_http_hello_create_loc_conf(ngx_conf_t *cf);

/* hello_string处理 */
static char *ngx_http_hello_string(ngx_conf_t *cf,ngx_command_t *cmd,void *conf);

/* hello_count处理 */
static char *ngx_http_hello_count(ngx_conf_t *cf,ngx_command_t *cmd,void *conf);

/* 模块命令 */
static ngx_command_t ngx_http_hello_commands[] = {
    {
        ngx_string("hello_string"),
        NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS|NGX_CONF_TAKE1,
        ngx_http_hello_string,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_hello_loc_conf_t,hello_string),
        NULL
    },
    {
        ngx_string("hello_count"),
        NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
        ngx_http_hello_count,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_hello_loc_conf_t,hello_count),
        NULL
    },
    ngx_null_command
};

static ngx_uint_t ngx_http_hello_visit_times = 0;

/* module content */
static ngx_http_module_t ngx_http_hello_module_ctx = {
    NULL,
    ngx_http_hello_init,
    NULL,
    NULL,
    NULL,
    NULL,
    ngx_http_hello_create_loc_conf,
    NULL
};

/* 模块信息 */
ngx_module_t ngx_http_hello_module = {
    NGX_MODULE_V1,
    &ngx_http_hello_module_ctx,
    ngx_http_hello_commands,
    NGX_HTTP_MODULE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING
};

/* 模块真正处理方法 */
static ngx_int_t ngx_http_hello_handler(ngx_http_request_t *r) {
    ngx_int_t                  rc;
    ngx_buf_t                 *b;
    ngx_chain_t                out;
    ngx_http_hello_loc_conf_t *loc_conf;
    u_char                     ngx_hello_content[1024] = {0};
    ngx_uint_t                 content_length = 0;

    loc_conf = ngx_http_get_module_loc_conf(r, ngx_http_hello_module);
    if (loc_conf->hello_string.len == 0) {
        ngx_log_error(NGX_LOG_EMERG, r->connection->log, 0, "hello string is empty");
        return NGX_DECLINED;
    }

    if (loc_conf->hello_count == NGX_CONF_UNSET || loc_conf->hello_count == 0) {
        ngx_sprintf(ngx_hello_content, "%s", loc_conf->hello_string.data);
    } else {
        ngx_sprintf(ngx_hello_content, "%s visit times : %d", loc_conf->hello_string.data, ++ngx_http_hello_visit_times);
    }

    content_length = ngx_strlen(ngx_hello_content);

    /* only response method 'get' and 'head' */
    if (!(r->method & (NGX_HTTP_GET | NGX_HTTP_HEAD))){
        return NGX_HTTP_NOT_ALLOWED;
    }

    /* discard body */
    rc = ngx_http_discard_request_body(r);
    if (rc != NGX_OK) {
        return rc;
    }

    ngx_str_set(&r->headers_out.content_type,"text/html");
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = content_length;

    if (r->method == NGX_HTTP_HEAD) {
        return ngx_http_send_header(r);
    }

    /* malloc content buf */
    b = ngx_palloc(r->pool, sizeof(ngx_buf_t));
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    out.buf = b;
    out.next = NULL;

    b->pos = ngx_hello_content;
    b->last = ngx_hello_content + content_length;
    b->memory = 1;
    b->last_buf = 1;

    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only){
        return rc;
    }

    /* call output filter */
    return ngx_http_output_filter(r, &out);
}

static ngx_int_t ngx_http_hello_init(ngx_conf_t *cf) {
    ngx_http_handler_pt       *h;
    ngx_http_core_main_conf_t *cmct;

    cmct = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    /* 把处理方法挂载到handlers链上 */
    h = ngx_array_push(&cmct->phases[NGX_HTTP_CONTENT_PHASE].handlers);
    if(h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_hello_handler;
    return NGX_OK;
}

static void *ngx_http_hello_create_loc_conf(ngx_conf_t *cf) {
    ngx_http_hello_loc_conf_t *loc_conf = NULL;
    loc_conf = ngx_pcalloc(cf->pool,sizeof(ngx_http_hello_loc_conf_t));
    if(loc_conf == NULL) {
        return NULL;
    }

    ngx_str_null(&loc_conf->hello_string);
    loc_conf->hello_count = NGX_CONF_UNSET;
    return loc_conf;
}

// static char *ngx_http_hello_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child) {
//     ngx_http_hello_loc_conf_t *prev, *chil;

//     prev = parent;
//     chil = child;

//     ngx_conf_merge_str_value(chil->hello_string, prev->hello_string, ngx_http_hello_default_string);
//     ngx_conf_merge_value(chil->hello_count, prev->hello_count, 0);

//     return NGX_OK;
// }

static char *ngx_http_hello_string(ngx_conf_t *cf,ngx_command_t *cmd,void *conf){
    ngx_http_hello_loc_conf_t *loc_conf;

    loc_conf = conf;
    char *rv = ngx_conf_set_str_slot(cf, cmd, conf);

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "hello string:%s", loc_conf->hello_string.data);
    return rv;
}

static char *ngx_http_hello_count(ngx_conf_t *cf,ngx_command_t *cmd,void *conf){
    ngx_http_hello_loc_conf_t *loc_conf;

    loc_conf = conf;
    char *rv = ngx_conf_set_flag_slot(cf, cmd, conf);
    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "hello count:%d", loc_conf->hello_count);
    return rv;
}