/*
 * Copyright 2014-2016 CyberVision, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef KAA_USE_SQLITE_LOG_STORAGE

#include <kaa/log/SQLiteDBLogStorage.hpp>

#include <kaa/logging/Log.hpp>
#include <kaa/log/LogRecord.hpp>
#include <kaa/common/exception/KaaException.hpp>
#include <kaa/KaaClientProperties.hpp>
#include "kaa/IKaaClientContext.hpp"

#include <cstdio>

#ifdef QNX_650_CPP11_TO_STRING_PATCH
#include <custom/string.h>
#else
#include <string>
#endif

#define KAA_LOGS_TABLE_NAME       "KAA_LOGS"
#define KAA_BUCKETS_TABLE_NAME    "KAA_BUCKETS"

#define KAA_LOGS_RECORD_ID_FIELD_NAME    "RECORD_ID"
#define KAA_LOGS_DATA_FIELD_NAME         "LOG_DATA"

#define KAA_BUCKETS_INNER_BUCKET_ID_FIELD_NAME    "IN_BUCKET_ID"
#define KAA_BUCKETS_OUTER_BUCKET_ID_FIELD_NAME    "OUT_BUCKET_ID"
#define KAA_BUCKETS_SIZE_IN_RECORDS_FIELD_NAME    "SIZE_IN_RECORDS"
#define KAA_BUCKETS_SIZE_IN_BYTES_FIELD_NAME      "SIZE_IN_BYTES"
#define KAA_BUCKETS_STATE_FIELD_NAME              "STATE" /* 0 - free, 1 - in use*/

#define KAA_BUCKET_ID_INDEX_NAME "KAA_BUCKET_ID_INDEX"

#define KAA_CREATE_LOGS_TABLE \
         "CREATE TABLE IF NOT EXISTS " KAA_LOGS_TABLE_NAME " ("  \
         KAA_LOGS_RECORD_ID_FIELD_NAME"                  INTEGER    PRIMARY KEY    AUTOINCREMENT," \
         KAA_BUCKETS_OUTER_BUCKET_ID_FIELD_NAME"         INTEGER," \
         KAA_LOGS_DATA_FIELD_NAME"                       BLOB);"

#define KAA_DROP_LOGS_TABLE \
        "DROP TABLE " KAA_LOGS_TABLE_NAME ";"

#define KAA_CREATE_BUCKETS_TABLE \
         "CREATE TABLE IF NOT EXISTS " KAA_BUCKETS_TABLE_NAME " ("  \
         KAA_BUCKETS_INNER_BUCKET_ID_FIELD_NAME "    INTEGER    PRIMARY KEY    AUTOINCREMENT," \
         KAA_BUCKETS_OUTER_BUCKET_ID_FIELD_NAME "    INTEGER    NOT NULL," \
         KAA_BUCKETS_SIZE_IN_RECORDS_FIELD_NAME "    INTEGER    DEFAULT 0," \
         KAA_BUCKETS_SIZE_IN_BYTES_FIELD_NAME "      INTEGER    DEFAULT 0," \
         KAA_BUCKETS_STATE_FIELD_NAME "              INTEGER    DEFAULT 0);"

#define KAA_DROP_BUCKETS_TABLE \
        "DROP TABLE " KAA_BUCKETS_TABLE_NAME ";"

#define KAA_CREATE_BUCKET_ID_INDEX \
    "CREATE INDEX IF NOT EXISTS " KAA_BUCKET_ID_INDEX_NAME " " \
    "ON " KAA_BUCKETS_TABLE_NAME " (" KAA_BUCKETS_OUTER_BUCKET_ID_FIELD_NAME ");"

#define KAA_INSERT_NEW_BUCKET \
    "INSERT INTO " KAA_BUCKETS_TABLE_NAME " " \
    " (" KAA_BUCKETS_OUTER_BUCKET_ID_FIELD_NAME ")" \
    "VALUES (?);"

#define KAA_INSERT_NEW_RECORD_IN_BUCKET \
    "INSERT INTO " KAA_LOGS_TABLE_NAME " " \
    " (" KAA_BUCKETS_OUTER_BUCKET_ID_FIELD_NAME "," KAA_LOGS_DATA_FIELD_NAME ")" \
    "VALUES (?,?);"


#define KAA_SELECT_BUCKET_LOG_RECORDS \
    "SELECT " KAA_LOGS_DATA_FIELD_NAME " "\
    "FROM " KAA_LOGS_TABLE_NAME " "\
    "WHERE " KAA_BUCKETS_OUTER_BUCKET_ID_FIELD_NAME " = ? " \
    "ORDER BY " KAA_LOGS_RECORD_ID_FIELD_NAME " ASC;"

#define KAA_MARK_ALL_BUCKETS_AS_FREE \
    "UPDATE " KAA_BUCKETS_TABLE_NAME " " \
    "SET " KAA_BUCKETS_STATE_FIELD_NAME " = 0;"

#define KAA_DELETE_BUCKET \
    "DELETE FROM " KAA_BUCKETS_TABLE_NAME " " \
    "WHERE " KAA_BUCKETS_OUTER_BUCKET_ID_FIELD_NAME " = ?;"

#define KAA_DELETE_BUCKET_RECORDS \
    "DELETE FROM " KAA_LOGS_TABLE_NAME " " \
    "WHERE " KAA_BUCKETS_OUTER_BUCKET_ID_FIELD_NAME " = ?;"

#define KAA_COUNT_RECORDS \
    "SELECT SUM(" KAA_BUCKETS_SIZE_IN_RECORDS_FIELD_NAME "), SUM(" KAA_BUCKETS_SIZE_IN_BYTES_FIELD_NAME ")" \
    "FROM " KAA_BUCKETS_TABLE_NAME ";"

#define KAA_GET_MAX_RECORD_COUNT_AND_BUCKET_SIZE \
    "SELECT MAX(" KAA_BUCKETS_SIZE_IN_RECORDS_FIELD_NAME "), MAX(" KAA_BUCKETS_SIZE_IN_BYTES_FIELD_NAME ")" \
    "FROM " KAA_BUCKETS_TABLE_NAME ";"

#define KAA_GET_THE_OLDEST_UNUSED_BUCKET \
    "SELECT * FROM " KAA_BUCKETS_TABLE_NAME " " \
    "WHERE " KAA_BUCKETS_STATE_FIELD_NAME " = 0 AND " KAA_BUCKETS_SIZE_IN_RECORDS_FIELD_NAME " != 0 " \
    "ORDER BY " KAA_BUCKETS_INNER_BUCKET_ID_FIELD_NAME " ASC " \
    "LIMIT 1;"

#define KAA_GET_THE_LATEST_BUCKET \
    "SELECT * FROM " KAA_BUCKETS_TABLE_NAME " " \
    "WHERE " KAA_BUCKETS_OUTER_BUCKET_ID_FIELD_NAME " = " \
                        "(SELECT MAX(" KAA_BUCKETS_OUTER_BUCKET_ID_FIELD_NAME ") " \
                         "FROM " KAA_BUCKETS_TABLE_NAME ");"

#define KAA_MARK_LOG_BUCKET_AS_IN_USE \
    "UPDATE " KAA_BUCKETS_TABLE_NAME " " \
    "SET " KAA_BUCKETS_STATE_FIELD_NAME " = 1 " \
    "WHERE " KAA_BUCKETS_OUTER_BUCKET_ID_FIELD_NAME " = ?;"

#define KAA_MARK_BUCKET_AS_FREE \
    "UPDATE " KAA_BUCKETS_TABLE_NAME " " \
    "SET " KAA_BUCKETS_STATE_FIELD_NAME " = 0 " \
    "WHERE " KAA_BUCKETS_OUTER_BUCKET_ID_FIELD_NAME " = ?;"

#define KAA_UPDATE_BUCKET_INFO \
    "UPDATE " KAA_BUCKETS_TABLE_NAME " " \
    "SET " KAA_BUCKETS_SIZE_IN_RECORDS_FIELD_NAME " = " KAA_BUCKETS_SIZE_IN_RECORDS_FIELD_NAME "+ 1, " \
     KAA_BUCKETS_SIZE_IN_BYTES_FIELD_NAME " = " KAA_BUCKETS_SIZE_IN_BYTES_FIELD_NAME "+ ? " \
    "WHERE " KAA_BUCKETS_OUTER_BUCKET_ID_FIELD_NAME " = ?;"

/*
 * OPTIMIZATION OPTIONS.
 */
#define KAA_SYNCHRONIZATION_OPTION        "PRAGMA synchronous=NORMAL"
#define KAA_MEMORY_JOURNAL_MODE_OPTION    "PRAGMA journal_mode=DELETE"
#define KAA_QNX_LOCKING_MODE              "PRAGMA locking_mode=EXCLUSIVE"
#define KAA_QNX_MEMORY_MAPPED_SIZE        "PRAGMA mmap_size=0"
#define KAA_MEMORY_TEMP_STORE_OPTION      "PRAGMA temp_store=MEMORY"
#define KAA_AUTO_VACUUM_OPTION            "PRAGMA auto_vacuum=FULL"

namespace kaa {

void SQLiteDBLogStorage::throwIfError(int errorCode, int expectedErrorCode)
{
    if (errorCode != expectedErrorCode) {
        throw std::exception(std::to_string(errorCode).c_str());
    }
}

class SQLiteStatement {
public:
    SQLiteStatement(sqlite3 *db, const char* sql)
    {
        if (!db || !sql) {
            throw KaaException("Failed to create sqlite3 statement: bad data");
        }

        int errorCode = sqlite3_prepare_v2(db, sql, -1, &stmt_, nullptr);

        SQLiteDBLogStorage::throwIfError(errorCode, SQLITE_OK);
    }

    ~SQLiteStatement()
    {
        if (stmt_) {
            sqlite3_finalize(stmt_);
        }
    }

    sqlite3_stmt *getStatement() { return stmt_; }

private:
    sqlite3_stmt *stmt_ = nullptr;
};

SQLiteDBLogStorage::SQLiteDBLogStorage(IKaaClientContext &context, std::size_t bucketSize, std::size_t bucketRecordCount)
    : dbName_(context.getProperties().getLogsDatabaseFileName()),
      maxBucketSize_(bucketSize), maxBucketRecordCount_(bucketRecordCount),
      context_(context)
{
    init(SQLiteOptimizationOptions::SQLITE_AUTO_VACUUM_FULL);
}

SQLiteDBLogStorage::SQLiteDBLogStorage(IKaaClientContext &context,
                                       const std::string& dbName, int optimizationMask,
                                       std::size_t bucketSize, std::size_t bucketRecordCount)
    : dbName_(dbName), maxBucketSize_(bucketSize), maxBucketRecordCount_(bucketRecordCount), context_(context)
{
    init(optimizationMask);
}

SQLiteDBLogStorage::~SQLiteDBLogStorage()
{
    closeDBConnection();
}

void SQLiteDBLogStorage::init(int optimizationMask)
{
    openDBConnection();
    applyDBOptimization(optimizationMask);

    initDBTables();
    retrieveConsumedSizeAndVolume();

    if (totalRecordCount_ > 0) {
        if (!truncateIfBucketSizeIncompatible()) {
            markBucketsAsFree();
        }
    }

    if (!retrieveLastBucketInfo()) {
        KAA_LOG_TRACE("No log buckets found");
        addNextBucket();
    }

    KAA_LOG_INFO(boost::format("%d log records in database (total size %d B)") % totalRecordCount_ % consumedMemory_);
}

bool SQLiteDBLogStorage::retrieveLastBucketInfo()
{
    SQLiteStatement getLatestBucketStmt(db_, KAA_GET_THE_LATEST_BUCKET);

    KAA_LOG_TRACE(boost::format("Step start: %s") % "bool SQLiteDBLogStorage::retrieveLastBucketInfo()");
    int errorCode = sqlite3_step(getLatestBucketStmt.getStatement());
    KAA_LOG_TRACE(boost::format("Step stop: %s") % "bool SQLiteDBLogStorage::retrieveLastBucketInfo()");

    switch (errorCode) {
    case SQLITE_DONE:
        break;
    case SQLITE_ROW:
        currentBucketId_ = sqlite3_column_int64(getLatestBucketStmt.getStatement(), 1);
        currentBucketRecordCount_ = sqlite3_column_int64(getLatestBucketStmt.getStatement(), 2);
        currentBucketSize_ = sqlite3_column_int64(getLatestBucketStmt.getStatement(), 3);

        KAA_LOG_TRACE(boost::format("Last bucket: %s") % bucketStatisticsToStr());

        return true;
    default:
        KAA_LOG_WARN(boost::format("Failed to get last log bucket, error %d") % errorCode);
        break;
    }

    return false;
}

void SQLiteDBLogStorage::retrieveConsumedSizeAndVolume()
{
    SQLiteStatement stmt(db_, KAA_COUNT_RECORDS);
    KAA_LOG_TRACE(boost::format("Step start: %s") % "void SQLiteDBLogStorage::retrieveConsumedSizeAndVolume()");
    int errorCode = sqlite3_step(stmt.getStatement());
    KAA_LOG_TRACE(boost::format("Step stop: %s") % "void SQLiteDBLogStorage::retrieveConsumedSizeAndVolume()");
    if (errorCode == SQLITE_ROW) {
        totalRecordCount_ = unmarkedRecordCount_ = sqlite3_column_int64(stmt.getStatement(), 0);
        consumedMemory_ = sqlite3_column_int64(stmt.getStatement(), 1);
    } else {
        KAA_LOG_ERROR("Failed to count log records in database");
    }
}

bool SQLiteDBLogStorage::truncateIfBucketSizeIncompatible()
{
    SQLiteStatement getMaxStmt(db_, KAA_GET_MAX_RECORD_COUNT_AND_BUCKET_SIZE);
    KAA_LOG_TRACE(boost::format("Step start 0: %s") % "bool SQLiteDBLogStorage::truncateIfBucketSizeIncompatible()");
    int errorCode = sqlite3_step(getMaxStmt.getStatement());
    KAA_LOG_TRACE(boost::format("Step stop 0: %s") % "bool SQLiteDBLogStorage::truncateIfBucketSizeIncompatible()");
    if (errorCode == SQLITE_ROW) {
        std::size_t maxBucketSizeInRecordCount = sqlite3_column_int64(getMaxStmt.getStatement(), 0);
        std::size_t maxBucketSizeInBytes = sqlite3_column_int64(getMaxStmt.getStatement(), 1);

        if (maxBucketSizeInRecordCount > maxBucketRecordCount_ || maxBucketSizeInBytes > maxBucketSize_) {
            KAA_LOG_INFO(boost::format("Truncating logs: current_max_bucket_size %1% bytes, current_max_bucket_records %2%, "
                                                             "db_max_bucket_size %3% bytes, db_max_bucket_records %4%")
                                                                % maxBucketSize_ % maxBucketRecordCount_
                                                                % maxBucketSizeInBytes % maxBucketSizeInRecordCount);

            SQLiteStatement dropBucketTableStmt(db_, KAA_DROP_BUCKETS_TABLE);
            KAA_LOG_TRACE(boost::format("Step start 1: %s") % "bool SQLiteDBLogStorage::truncateIfBucketSizeIncompatible()");
            sqlite3_step(dropBucketTableStmt.getStatement());
            KAA_LOG_TRACE(boost::format("Step stop 1: %s") % "bool SQLiteDBLogStorage::truncateIfBucketSizeIncompatible()");

            SQLiteStatement dropLogsTableStmt(db_, KAA_DROP_LOGS_TABLE);
            KAA_LOG_TRACE(boost::format("Step start 2: %s") % "bool SQLiteDBLogStorage::truncateIfBucketSizeIncompatible()");
            sqlite3_step(dropLogsTableStmt.getStatement());
            KAA_LOG_TRACE(boost::format("Step stop 2: %s") % "bool SQLiteDBLogStorage::truncateIfBucketSizeIncompatible()");

            totalRecordCount_ = unmarkedRecordCount_ = 0;
            consumedMemory_ = 0;

            initDBTables();

            return true;
        }
    }

    return false;
}

void SQLiteDBLogStorage::initDBTables()
{
    try {
        SQLiteStatement createBucketsTableStmt(db_, KAA_CREATE_BUCKETS_TABLE);
        KAA_LOG_TRACE(boost::format("Step start 1: %s") % "void SQLiteDBLogStorage::initDBTables()");
        int errorCode = sqlite3_step(createBucketsTableStmt.getStatement());
        KAA_LOG_TRACE(boost::format("Step stop 1: %s") % "void SQLiteDBLogStorage::initDBTables()");
        throwIfError(errorCode, SQLITE_DONE);

        KAA_LOG_TRACE("'" KAA_BUCKETS_TABLE_NAME "' table created");

        SQLiteStatement createLogsTableStmt(db_, KAA_CREATE_LOGS_TABLE);
        KAA_LOG_TRACE(boost::format("Step start 2: %s") % "void SQLiteDBLogStorage::initDBTables()");
        errorCode = sqlite3_step(createLogsTableStmt.getStatement());
        KAA_LOG_TRACE(boost::format("Step stop 2: %s") % "void SQLiteDBLogStorage::initDBTables()");
        throwIfError(errorCode, SQLITE_DONE);

        KAA_LOG_TRACE("'" KAA_LOGS_TABLE_NAME "' table created");

        //TODO: to increase performance need to create indexes on DB tables.
    } catch (std::exception& e) {
		std::string err{"Error code: " + std::string(e.what())};
        KAA_LOG_FATAL(boost::format("Failed to init log table: %s") % err);
        throw;
    }
}

void SQLiteDBLogStorage::applyDBOptimization(int mask)
{
    if (mask == SQLiteOptimizationOptions::SQLITE_NO_OPTIMIZATIONS) {
        KAA_LOG_INFO("No database optimization option is used");
        return;
    }

    if (mask & SQLiteOptimizationOptions::SQLITE_SYNCHRONOUS_FLAG) {
        sqlite3_exec(db_, KAA_SYNCHRONIZATION_OPTION, nullptr, nullptr, nullptr);
        KAA_LOG_INFO(boost::format("Applied '%s' optimization") % KAA_SYNCHRONIZATION_OPTION);
    }
    if (mask & SQLiteOptimizationOptions::SQLITE_MEMORY_JOURNAL_MODE) {
        sqlite3_exec(db_, KAA_MEMORY_JOURNAL_MODE_OPTION, nullptr, nullptr, nullptr);
        KAA_LOG_INFO(boost::format("Applied '%s' optimization") % KAA_MEMORY_JOURNAL_MODE_OPTION);
        /*#ifdef __QNXNTO__
        sqlite3_exec(db_, KAA_QNX_MEMORY_MAPPED_SIZE, nullptr, nullptr, nullptr);
        KAA_LOG_INFO(boost::format("Applied '%s' optimization") % KAA_QNX_MEMORY_MAPPED_SIZE);
        sqlite3_exec(db_, KAA_QNX_LOCKING_MODE, nullptr, nullptr, nullptr);
        KAA_LOG_INFO(boost::format("Applied '%s' optimization") % KAA_QNX_LOCKING_MODE);
        #endif*/
    }
    if (mask & SQLiteOptimizationOptions::SQLITE_MEMORY_TEMP_STORE) {
        sqlite3_exec(db_, KAA_MEMORY_TEMP_STORE_OPTION, nullptr, nullptr, nullptr);
        KAA_LOG_INFO(boost::format("Applied '%s' optimization") % KAA_MEMORY_TEMP_STORE_OPTION);
    }
    if (mask & SQLiteOptimizationOptions::SQLITE_AUTO_VACUUM_FULL) {
        sqlite3_exec(db_, KAA_AUTO_VACUUM_OPTION, nullptr, nullptr, nullptr);
        KAA_LOG_INFO(boost::format("Applied '%s' optimization") % KAA_AUTO_VACUUM_OPTION);
    }
}

void SQLiteDBLogStorage::markBucketsAsFree()
{
    try {
        SQLiteStatement stmt(db_, KAA_MARK_ALL_BUCKETS_AS_FREE);
        KAA_LOG_TRACE(boost::format("Step start: %s") % "void SQLiteDBLogStorage::markBucketsAsFree()");
        int errorCode = sqlite3_step(stmt.getStatement());
        KAA_LOG_TRACE(boost::format("Step stop: %s") % "void SQLiteDBLogStorage::markBucketsAsFree()");
        throwIfError(errorCode, SQLITE_DONE);

        KAA_LOG_INFO(boost::format("Mark %1% bucket(s) as free") % sqlite3_changes(db_));
    } catch (std::exception& e) {
		std::string err{"Error code: " + std::string(e.what())};
        KAA_LOG_FATAL(boost::format("Failed to mark bucket(s) as free: %s") % err);
        throw;
    }
}

void SQLiteDBLogStorage::markBucketAsInUse(std::int32_t id)
{
    try {
        SQLiteStatement stmt(db_, KAA_MARK_LOG_BUCKET_AS_IN_USE);

        int errorCode = sqlite3_bind_int64(stmt.getStatement(), 1, id);
        throwIfError(errorCode, SQLITE_OK);

        KAA_LOG_TRACE(boost::format("Step start: %s") % "void SQLiteDBLogStorage::markBucketAsInUse(std::int32_t id)");
        errorCode = sqlite3_step(stmt.getStatement());
        KAA_LOG_TRACE(boost::format("Step stop: %s") % "void SQLiteDBLogStorage::markBucketAsInUse(std::int32_t id)");
        throwIfError(errorCode, SQLITE_DONE);

        KAA_LOG_TRACE(boost::format("Mark log bucket %d as in use") % id);
    } catch (std::exception& e) {
		std::string err{"Error code: " + std::string(e.what())};
        KAA_LOG_ERROR(boost::format("Failed to mark log bucket %d as in use: %s") % id % err);
        throw;
    }
}

void SQLiteDBLogStorage::openDBConnection()
{
    KAA_LOG_TRACE(boost::format("Going to connect to '%s' log database") % dbName_);

    while(true)
    {
        int errorCode = sqlite3_open(dbName_.c_str(), &db_);
        if(errorCode == SQLITE_CORRUPT)
        {
            if(std::remove(dbName_.c_str()))
            {
                KAA_LOG_INFO(boost::format("'%s' log database is corrupted") % dbName_);
                continue;
            }
            else
            {
                KAA_LOG_ERROR(boost::format("Cannot delete corrupted '%s' log database") % dbName_);
            }
        }
        throwIfError(errorCode, SQLITE_OK);
        break;
    }

    KAA_LOG_INFO(boost::format("Connected to '%s' log database") % dbName_);
}

void SQLiteDBLogStorage::closeDBConnection()
{
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

BucketInfo SQLiteDBLogStorage::addLogRecord(LogRecord&& record)
{
    auto recordSize = record.getSize();
    if (recordSize > maxBucketSize_) {
        KAA_LOG_WARN(boost::format("Failed to add log record: record_size %1%B, max_bucket_size %2%B")
                                                                        % recordSize % maxBucketSize_);
        throw KaaException("Too big log record");
    }

    KAA_MUTEX_LOCKING("sqliteLogStorageGuard_");
    KAA_MUTEX_UNIQUE_DECLARE(storageGuardLock, sqliteLogStorageGuard_);
    KAA_MUTEX_LOCKED("sqliteLogStorageGuard_");

    if (checkBucketOverflow(record)) {
        KAA_LOG_TRACE(boost::format("Need to add new log bucket. Current %s. "
                                    "Log record size %d, max_logs %d, max_size % d")
                                        % bucketStatisticsToStr()
                                        % record.getSize()
                                        % maxBucketRecordCount_
                                        % maxBucketSize_);
        addNextBucket();
    }

    try {
        SQLiteStatement insertLogRecordStmt(db_, KAA_INSERT_NEW_RECORD_IN_BUCKET);

        int errorCode = sqlite3_bind_int(insertLogRecordStmt.getStatement(), 1, currentBucketId_);
        throwIfError(errorCode, SQLITE_OK);

        errorCode = sqlite3_bind_blob(insertLogRecordStmt.getStatement(), 2, record.getData().data(), record.getSize(), SQLITE_STATIC);
        throwIfError(errorCode, SQLITE_OK);

        SQLiteStatement updateBucketInfoStmt(db_, KAA_UPDATE_BUCKET_INFO);

        errorCode = sqlite3_bind_int(updateBucketInfoStmt.getStatement(), 1, record.getSize());
        throwIfError(errorCode, SQLITE_OK);

        errorCode = sqlite3_bind_int(updateBucketInfoStmt.getStatement(), 2, currentBucketId_);
        throwIfError(errorCode, SQLITE_OK);

        KAA_LOG_TRACE(boost::format("Step start 1: %s") % "BucketInfo SQLiteDBLogStorage::addLogRecord(LogRecord&& record)");
        errorCode = sqlite3_step(insertLogRecordStmt.getStatement());
        KAA_LOG_TRACE(boost::format("Step stop 1: %s") % "BucketInfo SQLiteDBLogStorage::addLogRecord(LogRecord&& record)");
        throwIfError(errorCode, SQLITE_DONE);

        KAA_LOG_TRACE(boost::format("Step start 2: %s") % "BucketInfo SQLiteDBLogStorage::addLogRecord(LogRecord&& record)");
        errorCode = sqlite3_step(updateBucketInfoStmt.getStatement());
        KAA_LOG_TRACE(boost::format("Step stop 2: %s") % "BucketInfo SQLiteDBLogStorage::addLogRecord(LogRecord&& record)");
        throwIfError(errorCode, SQLITE_DONE);

        ++unmarkedRecordCount_;
        ++totalRecordCount_;
        consumedMemory_ += record.getSize();

        ++currentBucketRecordCount_;
        currentBucketSize_ += record.getSize();

        KAA_LOG_TRACE(boost::format("Log record (%d bytes) added to %s. %s")
                                        % record.getSize() % bucketStatisticsToStr() % storageStatisticsToStr());
    } catch (std::exception& e) {
		std::string err{"Error code: " + std::string(e.what())};
        KAA_LOG_ERROR(boost::format("Failed to add log record: %s") % err);
        throw;
    }

    return BucketInfo(currentBucketId_, currentBucketRecordCount_);
}

LogBucket SQLiteDBLogStorage::getNextBucket()
{
    try {
        SQLiteStatement getOldestBucketStmt(db_, KAA_GET_THE_OLDEST_UNUSED_BUCKET);

        KAA_MUTEX_LOCKING("sqliteLogStorageGuard_");
        KAA_MUTEX_UNIQUE_DECLARE(storageGuardLock, sqliteLogStorageGuard_);
        KAA_MUTEX_LOCKED("sqliteLogStorageGuard_");

        KAA_LOG_TRACE(boost::format("Step start 0: %s") % "LogBucket SQLiteDBLogStorage::getNextBucket()");
        int errorCode = sqlite3_step(getOldestBucketStmt.getStatement());
        KAA_LOG_TRACE(boost::format("Step stop 0: %s") % "LogBucket SQLiteDBLogStorage::getNextBucket()");
        if (errorCode == SQLITE_DONE) {
            KAA_LOG_DEBUG("No unused log bucket found");
            return LogBucket();
        }

        throwIfError(errorCode, SQLITE_ROW);

        std::int32_t bucketId = sqlite3_column_int64(getOldestBucketStmt.getStatement(), 1);
        std::size_t bucketSizeInRecords = sqlite3_column_int64(getOldestBucketStmt.getStatement(), 2);
        std::size_t bucketSizeInBytes = sqlite3_column_int64(getOldestBucketStmt.getStatement(), 3);

        /*
         * Only one zero-size bucket can exist in the database.
         * This is a bucket that is currently used and yet no logs are added.
         * So we don't expect to get it, because it must be filtered by a SQL query.
         */
        if (!bucketSizeInRecords || !bucketSizeInBytes) {
            KAA_LOG_ERROR(boost::format("Zero-sized log bucket found, id %d") % bucketId);

            KAA_MUTEX_UNLOCKING("sqliteLogStorageGuard_");
            storageGuardLock.unlock();
            KAA_MUTEX_UNLOCKED("sqliteLogStorageGuard_");

            removeBucket(bucketId);

            throw KaaException("Zero-sized log bucket found");
        }

        SQLiteStatement getBucketLogRecordsStmt(db_, KAA_SELECT_BUCKET_LOG_RECORDS);

        errorCode = sqlite3_bind_int(getBucketLogRecordsStmt.getStatement(), 1, bucketId);
        throwIfError(errorCode, SQLITE_OK);

        std::list<LogRecord> records;

        KAA_LOG_TRACE(boost::format("Step start while: %s") % "LogBucket SQLiteDBLogStorage::getNextBucket()");
        while (SQLITE_ROW == (errorCode = sqlite3_step(getBucketLogRecordsStmt.getStatement()))) {
            const void *recordData = sqlite3_column_blob(getBucketLogRecordsStmt.getStatement(), 0);
            int recordDataSize = sqlite3_column_bytes(getBucketLogRecordsStmt.getStatement(), 0);
            records.emplace_back(reinterpret_cast<const std::uint8_t *>(recordData), recordDataSize);
        }
        KAA_LOG_TRACE(boost::format("Step stop while: %s") % "LogBucket SQLiteDBLogStorage::getNextBucket()");

        throwIfError(errorCode, SQLITE_DONE);

        markBucketAsInUse(bucketId);

        unmarkedRecordCount_ -= bucketSizeInRecords;
        consumedMemory_ -= bucketSizeInBytes;
        consumedMemoryStorage_.insert(std::make_pair(bucketId, InnerBucketInfo(bucketSizeInBytes, bucketSizeInRecords)));

        KAA_LOG_INFO(boost::format("Get log bucket: id %d, logs %d, size %d. %s")
                                        % bucketId % bucketSizeInRecords % bucketSizeInBytes % storageStatisticsToStr());

        if (currentBucketId_ == bucketId) {
            KAA_LOG_TRACE("Need to add new log bucket: all bucket are in use");
            addNextBucket();
        }

        return LogBucket(bucketId, std::move(records));
    } catch (std::exception& e) {
		std::string err{"Error code: " + std::string(e.what())};
        KAA_LOG_ERROR(boost::format("Failed to get log bucket: %s") % err);
        throw;
    }

    return LogBucket();
}


void SQLiteDBLogStorage::removeBucket(std::int32_t bucketId)
{
    try {
        SQLiteStatement deleteBucketStmt(db_, KAA_DELETE_BUCKET);

        int errorCode = sqlite3_bind_int64(deleteBucketStmt.getStatement(), 1, bucketId);
        throwIfError(errorCode, SQLITE_OK);

        SQLiteStatement deleteBucketRecordsStmt(db_, KAA_DELETE_BUCKET_RECORDS);

        errorCode = sqlite3_bind_int64(deleteBucketRecordsStmt.getStatement(), 1, bucketId);
        throwIfError(errorCode, SQLITE_OK);

        KAA_MUTEX_LOCKING("sqliteLogStorageGuard_");
        KAA_MUTEX_UNIQUE_DECLARE(storageGuardLock, sqliteLogStorageGuard_);
        KAA_MUTEX_LOCKED("sqliteLogStorageGuard_");

        KAA_LOG_TRACE(boost::format("Step start 1: %s") % "void SQLiteDBLogStorage::removeBucket(std::int32_t bucketId)");
        errorCode = sqlite3_step(deleteBucketStmt.getStatement());
        KAA_LOG_TRACE(boost::format("Step stop 1: %s") % "void SQLiteDBLogStorage::removeBucket(std::int32_t bucketId)");
        throwIfError(errorCode, SQLITE_DONE);

        KAA_LOG_TRACE(boost::format("Step start 2: %s") % "void SQLiteDBLogStorage::removeBucket(std::int32_t bucketId)");
        errorCode = sqlite3_step(deleteBucketRecordsStmt.getStatement());
        KAA_LOG_TRACE(boost::format("Step stop 2: %s") % "void SQLiteDBLogStorage::removeBucket(std::int32_t bucketId)");
        throwIfError(errorCode, SQLITE_DONE);

        auto removedRecordsCount = sqlite3_changes(db_);
        totalRecordCount_ -= removedRecordsCount;
        consumedMemoryStorage_.erase(bucketId);

        consumedMemoryStorage_.rehash(0);

        KAA_LOG_INFO(boost::format("Removed %d log records, bucket id %d. %s")
                                    % removedRecordsCount % bucketId % storageStatisticsToStr());
    } catch (std::exception& e) {
		std::string err{"Error code: " + std::string(e.what())};
        KAA_LOG_ERROR(boost::format("Failed to remove log bucket/records by bucket id %d: %s")
                                                                        % bucketId % err);
        throw;
    }
}

void SQLiteDBLogStorage::rollbackBucket(std::int32_t bucketId)
{
    try {
        SQLiteStatement stmt(db_, KAA_MARK_BUCKET_AS_FREE);

        int errorCode = sqlite3_bind_int64(stmt.getStatement(), 1, bucketId);
        throwIfError(errorCode, SQLITE_OK);

        KAA_MUTEX_LOCKING("sqliteLogStorageGuard_");
        KAA_MUTEX_UNIQUE_DECLARE(storageGuardLock, sqliteLogStorageGuard_);
        KAA_MUTEX_LOCKED("sqliteLogStorageGuard_");

        KAA_LOG_TRACE(boost::format("Step start: %s") % "void SQLiteDBLogStorage::rollbackBucket(std::int32_t bucketId)");
        errorCode = sqlite3_step(stmt.getStatement());
        KAA_LOG_TRACE(boost::format("Step stop: %s") % "void SQLiteDBLogStorage::rollbackBucket(std::int32_t bucketId)");
        throwIfError(errorCode, SQLITE_DONE);

        auto it = consumedMemoryStorage_.find(bucketId);
        if (it != consumedMemoryStorage_.end()) {
            consumedMemory_ += it->second.sizeInBytes_;
            unmarkedRecordCount_ += it->second.sizeInLogs_;
            consumedMemoryStorage_.erase(it);
        }

        consumedMemoryStorage_.rehash(0);

        KAA_LOG_INFO(boost::format("Bucket %d is rolled back. %s") % bucketId % storageStatisticsToStr());
    } catch (std::exception& e) {
		std::string err{"Error code: " + std::string(e.what())};
        KAA_LOG_ERROR(boost::format("Failed to roll back bucket %d: %s") % bucketId % err);
		throw;
    }
}


std::size_t SQLiteDBLogStorage::getRecordsCount()
{
    KAA_MUTEX_LOCKING("sqliteLogStorageGuard_");
    KAA_MUTEX_UNIQUE_DECLARE(storageGuardLock, sqliteLogStorageGuard_);
    KAA_MUTEX_LOCKED("sqliteLogStorageGuard_");

    return unmarkedRecordCount_;
}

std::size_t SQLiteDBLogStorage::getConsumedVolume()
{
    KAA_MUTEX_LOCKING("sqliteLogStorageGuard_");
    KAA_MUTEX_UNIQUE_DECLARE(storageGuardLock, sqliteLogStorageGuard_);
    KAA_MUTEX_LOCKED("sqliteLogStorageGuard_");

    return consumedMemory_;
}

void SQLiteDBLogStorage::addNextBucket()
{
    /*
     * This function should be called only from a one thread simultaneously.
     */

    auto newBucketId = currentBucketId_ + 1;

    try {
        SQLiteStatement insertStmt(db_, KAA_INSERT_NEW_BUCKET);

        int errorCode = sqlite3_bind_int(insertStmt.getStatement(), 1, newBucketId);
        throwIfError(errorCode, SQLITE_OK);

        KAA_LOG_TRACE(boost::format("Step start: %s") % "void SQLiteDBLogStorage::addNextBucket()");
        errorCode = sqlite3_step(insertStmt.getStatement());
        KAA_LOG_TRACE(boost::format("Step stop: %s") % "void SQLiteDBLogStorage::addNextBucket()");
        throwIfError(errorCode, SQLITE_DONE);

        currentBucketId_ = newBucketId;
        currentBucketSize_ = currentBucketRecordCount_ = 0;

        KAA_LOG_DEBUG(boost::format("Add new bucket. %s") % bucketStatisticsToStr());
    } catch (std::exception& e) {
		std::string err{"Error code: " + std::string(e.what())};
        KAA_LOG_ERROR(boost::format("Failed to add new bucket into database: %s") % err);
        throw;
    }
}

} /* namespace kaa */

#endif /* KAA_USE_SQLITE_LOG_STORAGE */
