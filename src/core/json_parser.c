/* src/core/json_parser.c
 *
 * Sigma.Build JSON Parser
 * A JSON parser for Sigma.Build.
 *
 * David Boarman
 * 2025-05-25
 *
 */

#include "json_parser.h"
#include <lib/cjson/cJSON.h>
#include <string.h>
#include <stdlib.h>

#define JSON_PARSER_VERSION "0.00.01"

static char **parse_string_array(cJSON *array) {
    if (!array || !cJSON_IsArray(array)) return NULL;
    int count = cJSON_GetArraySize(array);
    char **result = malloc((count + 1) * sizeof(char*));
    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(array, i);
        result[i] = cJSON_IsString(item) ? strdup(item->valuestring) : strdup("");
    }
    result[count] = NULL;
    return result;
}

static BuildTarget parse_target(cJSON *target_json) {
    BuildTarget target = malloc(sizeof(struct build_target_s));
    if (!target) {
        Logger.debug(stderr, LOG_NORMAL, DBG_ERROR, "Failed to allocate memory for build target.");
        return NULL;
    }
    cJSON *name = cJSON_GetObjectItemCaseSensitive(target_json, "name");
    cJSON *type = cJSON_GetObjectItemCaseSensitive(target_json, "type");
    cJSON *sources = cJSON_GetObjectItemCaseSensitive(target_json, "sources");
    cJSON *build_dir = cJSON_GetObjectItemCaseSensitive(target_json, "build_dir");
    cJSON *compiler = cJSON_GetObjectItemCaseSensitive(target_json, "compiler");
    cJSON *c_flags = cJSON_GetObjectItemCaseSensitive(target_json, "compiler_flags");
    cJSON *ld_flags = cJSON_GetObjectItemCaseSensitive(target_json, "linker_flags");

    target->name = cJSON_IsString(name) ? strdup(name->valuestring) : NULL;
    target->type = cJSON_IsString(type) ? strdup(type->valuestring) : NULL;
    target->sources = parse_string_array(sources);
    target->build_dir = cJSON_IsString(build_dir) ? strdup(build_dir->valuestring) : NULL;
    target->compiler = cJSON_IsString(compiler) ? strdup(compiler->valuestring) : NULL;
    target->c_flags = parse_string_array(c_flags);
    target->ld_flags = parse_string_array(ld_flags);

    if (!target->name || !target->type || !target->sources || !target->build_dir || !target->compiler) {
        Logger.debug(stderr, LOG_NORMAL, DBG_ERROR, "Missing required fields in build target.");
        free(target->name);
        free(target->type);
        for (char **src = target->sources; src && *src; src++) free(*src);
        free(target->sources);
        free(target->build_dir);
        free(target->compiler);
        for (char **flag = target->c_flags; flag && *flag; flag++) free(*flag);
        free(target->c_flags);
        for (char **flag = target->ld_flags; flag && *flag; flag++) free(*flag);
        free(target->ld_flags);
        free(target);
        return NULL;
    }
    return target;
}

BuildConfig parser_build_config(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        Logger.debug(stderr, LOG_NORMAL, DBG_ERROR, "Failed to open config: %s", filename);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buffer = malloc(size + 1);
    if (!buffer) {
        fclose(fp);
        Logger.debug(stderr, LOG_NORMAL, DBG_ERROR, "Memory allocation failed for config buffer.");
        return NULL;
    }
    fread(buffer, 1, size, fp);
    buffer[size] = '\0';
    fclose(fp);

    cJSON *json = cJSON_Parse(buffer);
    free(buffer);
    if (!json) {
        Logger.debug(stderr, LOG_NORMAL, DBG_ERROR, "JSON parse error: %s", cJSON_GetErrorPtr());
        return NULL;
    }

    BuildConfig config = malloc(sizeof(struct build_config_s));
    if (!config) {
        cJSON_Delete(json);
        Logger.debug(stderr, LOG_NORMAL, DBG_ERROR, "Memory allocation failed for config.");
        return NULL;
    }
    cJSON *name = cJSON_GetObjectItemCaseSensitive(json, "name");
    cJSON *log_file = cJSON_GetObjectItemCaseSensitive(json, "log_file");
    cJSON *targets = cJSON_GetObjectItemCaseSensitive(json, "targets");
    cJSON *variables = cJSON_GetObjectItemCaseSensitive(json, "variables");

    config->name = cJSON_IsString(name) ? strdup(name->valuestring) : NULL;
    config->log_file = cJSON_IsString(log_file) ? strdup(log_file->valuestring) : NULL;

    // Parse variables
    int var_count = variables && cJSON_IsObject(variables) ? cJSON_GetArraySize(variables) : 0;
    config->variables = malloc((var_count + 1) * sizeof(char*));
    if (variables && cJSON_IsObject(variables)) {
        int i = 0;
        cJSON *var;
        cJSON_ArrayForEach(var, variables) {
            char buf[256];
            snprintf(buf, sizeof(buf), "%s=%s", var->string, var->valuestring);
            config->variables[i++] = strdup(buf);
        }
        config->variables[i] = NULL;
    } else {
        config->variables[0] = NULL;
    }

    // Parse targets
    int target_count = cJSON_IsArray(targets) ? cJSON_GetArraySize(targets) : 0;
    config->targets = malloc((target_count + 1) * sizeof(BuildTarget));
    for (int i = 0; i < target_count; i++) {
        cJSON *target_json = cJSON_GetArrayItem(targets, i);
        config->targets[i] = parse_target(target_json);
        if (!config->targets[i]) {
            for (int j = 0; j < i; j++) {
                JParse.dispose(config->targets[j]);
            }
            free(config->targets);
            free(config->name);
            free(config->log_file);
            for (char **var = config->variables; var && *var; var++) free(*var);
            free(config->variables);
            free(config);
            cJSON_Delete(json);
            return NULL;
        }
    }
    config->targets[target_count] = NULL;

    cJSON_Delete(json);
    Logger.fwriteln(stdout, "Parsed config: %s", filename);
    return config;
}

void parser_dispose_config(BuildConfig config) {
    if (!config) return;
    free(config->name);
    free(config->log_file);
    for (char **var = config->variables; var && *var; var++) free(*var);
    free(config->variables);
    for (BuildTarget *target = config->targets; target && *target; target++) {
        free((*target)->name);
        free((*target)->type);
        for (char **src = (*target)->sources; src && *src; src++) free(*src);
        free((*target)->sources);
        free((*target)->build_dir);
        free((*target)->compiler);
        for (char **flag = (*target)->c_flags; flag && *flag; flag++) free(*flag);
        free((*target)->c_flags);
        for (char **flag = (*target)->ld_flags; flag && *flag; flag++) free(*flag);
        free((*target)->ld_flags);
        free(*target);
    }
    free(config->targets);
    free(config);
}

const struct IParser JParse = {
    .parse = parser_build_config,
    .dispose = parser_dispose_config
};