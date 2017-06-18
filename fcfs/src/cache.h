#ifndef CACHE_H
#define CACHE_H

#include <sys/stat.h>
#include "dev.h"

/**
    Инициализация системы кэша
*/
void
cache_init(void);

/**
    Деинициализация системы кэша
*/
void
cache_destroy(void);

/**
    Инициализация системы кэширования метаданных файлов
*/
void
cache_stat_init(void);

/**
    Деинициализация системы кэширования метаданных файлов
*/
void
cache_stat_destroy(void);

/**
    Инициализация системы кэширования списков блоков файлов
*/
void
cache_seq_init(void);

/**
    Деинициализация системы кэширования списков блоков файлов
*/
void
cache_seq_destroy(void);

/**
    Добавление метаданных файла в кэш
    @param [in] path указатель на строку пути к файлу
    @param [in] st указатель на структуру stat с данными файла
*/
void
cache_stat_add(const char *path, struct stat *st);

/**
    Получения метаданных файла из кэша
    @param [in] path указатель на строку пути к файлу
    @return NULL - если метаданных нет в кэше, иначе указатель на заполненную структуру stat
*/
struct stat *
cache_stat_get(const char *path);

/**
    Добавление списка блоков файла в кэш
    @param [in] id идентифиатор файла
    @param [in] l указатель на голову списка блоков файлов
*/
void
cache_seq_add(int id, dev_blk_info_t *l);

/**
    Удаление списка блоков файла из кэша
    @param [in] id идентифиатор файла
*/
void
cache_seq_rm(int id);

/**
    Получение списка блоков файла из кэша
    @param [in] id идентифиатор файла
    @param [out] sz длина списка блоков
    @return NULL - если id не в кэше, иначе указатель на голову списка блоков
*/
dev_blk_info_t *
cache_seq_get(int id, int *sz);

#endif
