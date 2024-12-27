//
// Created by eli on 5/20/2022.
//

#ifndef BRANEENGINE_PREPPEDSQLCALL_H
#define BRANEENGINE_PREPPEDSQLCALL_H

#include "runtime/runtime.h"
#include "sqlite3.h"

#include <cassert>
#include <functional>
#include <stdexcept>
#include <string>
#include <tuple>

using sqlINT = int32_t;
using sqlINT64 = int64_t;
using sqlFLOAT = double;
using sqlTEXT = std::string;

template<typename... Args>
class PreppedSQLCall
{
    sqlite3_stmt* stmt = nullptr;

    void checkBindArg(int res)
    {
        if(res != SQLITE_OK)
            throw std::runtime_error("could not bind argument, returned with result: " + std::to_string(res));
    }

    template<typename... Types>
    void bindArgs(const Types... args)
    {
        bindArg(1, args...);
    }

    template<typename... Types>
    void bindArg(int index, const sqlTEXT& arg, Types... args)
    {
        checkBindArg(sqlite3_bind_text(stmt, index, arg.c_str(), arg.size(), SQLITE_TRANSIENT));
        if constexpr(sizeof...(Types))
            bindArg(index + 1, args...);
    }

    template<typename... Types>
    void bindArg(int index, sqlINT arg, Types... args)
    {
        checkBindArg(sqlite3_bind_int(stmt, index, arg));
        if constexpr(sizeof...(Types))
            bindArg(index + 1, args...);
    }

    template<typename... Types>
    void bindArg(int index, sqlINT64 arg, Types... args)
    {
        checkBindArg(sqlite3_bind_int64(stmt, index, arg));
        if constexpr(sizeof...(Types))
            bindArg(index + 1, args...);
    }

    template<typename... Types>
    void bindArg(int index, sqlFLOAT arg, Types... args)
    {
        checkBindArg(sqlite3_bind_double(stmt, index, arg));
        if constexpr(sizeof...(Types))
            bindArg(index + 1, args...);
    }

    template<typename... Types>
    void getColumns(std::tuple<Types...>& columns)
    {
        getColumn<0, Types...>(columns);
    }

    template<size_t index, typename... Types>
    void getColumn(std::tuple<Types...>& columns)
    {
        getColumn(std::get<index>(columns), index);

        if constexpr(index + 1 < sizeof...(Types))
            getColumn<index + 1, Types...>(columns);
    }

    void getColumn(sqlINT& value, int index) { value = sqlite3_column_int(stmt, index); }

    void getColumn(sqlINT64& value, int index) { value = sqlite3_column_int64(stmt, index); }

    void getColumn(sqlFLOAT& value, int index) { value = sqlite3_column_double(stmt, index); }

    void getColumn(sqlTEXT& value, int index)
    {
        const char* data = (const char*)sqlite3_column_text(stmt, index);
        size_t size = sqlite3_column_bytes(stmt, index);
        value = std::string(data, size);
    }

  public:
    ~PreppedSQLCall()
    {
        if(stmt)
            sqlite3_finalize(stmt);
    }

    void initialize(const std::string& sql, sqlite3* db)
    {
        int res = sqlite3_prepare_v2(db, sql.data(), sql.size(), &stmt, nullptr);
        if(res != SQLITE_OK)
        {
            Runtime::error("Could not initialize prepared sql call: " + sql +
                           "\nsqlite returned: " + std::to_string(res));
            throw std::runtime_error("Could not initialize prepared sql call: " + sql +
                                     "\nsqlite returned: " + std::to_string(res));
        }
    }

    void run(const Args... args)
    {
        assert(stmt);
        bindArgs(args...);
        int result = SQLITE_ROW;
        while(result == SQLITE_ROW)
            result = sqlite3_step(stmt);
        sqlite3_reset(stmt);
        if(result != SQLITE_DONE)
            throw std::runtime_error("Prepared SQL statement failed with return value: " + std::to_string(result));
    }

    template<typename Runnable>
    void run(const Args... args, Runnable r)
    {
        run(args..., std::function(r));
    }

    template<typename... Columns>
    void run(const Args... args, std::function<void(Columns...)> f)
    {
        assert(stmt);
        bindArgs(args...);
        int result = SQLITE_ROW;
        while(result == SQLITE_ROW)
        {
            result = sqlite3_step(stmt);
            if(result == SQLITE_ROW)
            {
                std::tuple<Columns...> columns;
                getColumns(columns);
                std::apply(f, columns);
            }
        }
        sqlite3_reset(stmt);
        if(result != SQLITE_DONE)
            throw std::runtime_error("Prepared SQL statement failed with return value: " + std::to_string(result));
    }
};

#endif // BRANEENGINE_PREPPEDSQLCALL_H
