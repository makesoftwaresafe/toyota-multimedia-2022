#ifndef IPP_IAP2_DATABASE_H
#define IPP_IAP2_DATABASE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <sys/stat.h>

#include "adit_typedef_linux.h"
#include "iap2_defines.h"
#include "iPodPlayerCoreDef.h"

#define IPOD_PLAYER_IAP2_DB_BACKUP_ALL_PAGES -1
#define IPOD_PLAYER_IAP2_DB_STATEMENT_MAX 1024
#define IPOD_PLAYER_IAP2_DB_MAX_DEVICEID_LEN 256
#define IPOD_PLAYER_IAP2_DB_LOCAL_DEVICE_FILE_NAME "%s/ipp_%d.db"
#define IPOD_PLAYER_IAP2_DB_APPLE_MUSIC_RADIO_FILE_NAME "%s/ipp_amr_%d.db"
#define IPOD_PLAYER_IAP2_DB_QUERY_REVISION "SELECT Revision FROM MediaInfo;"
#define IPOD_PLAYER_IAP2_DB_STRING_MAX_LEN 256
#define IPOD_PLAYER_IAP2_DB_TIMEOUT 5000

#define IPP_NO_MEDIA_TYPE           99
#define IPP_MEDIA_TYPE_MUSIC        0
#define IPP_MEDIA_TYPE_PODCAST      1
#define IPP_MEDIA_TYPE_AUDIOBOOK    2  
#define IPP_MEDIA_TYPE_ITUNESU      3


/* ################# SQL statement for initialize MediaInfo ############### */
#define IPOD_PLAYER_IAP2_DB_CREATE_MEDIAINFO_TABLE "CREATE TABLE MediaInfo(DeviceID text PRIMARY KEY, MediaID text, MediaName text, MediaType text, Revision text, Progress integer, IsHiding integer);"

#define IPOD_PLAYER_IAP2_DB_CREATE_MEDIAITEM_TABLE "CREATE TABLE MediaItem( TrackID integer PRIMARY KEY,               \
                                                                            TrackTitle text,                           \
                                                                            AlbumID integer,                           \
                                                                            AlbumTitle text,                           \
                                                                            AlbumDiscCount integer,                    \
                                                                            AlbumDiscNumber integer,                   \
                                                                            AlbumArtistID integer,                     \
                                                                            AlbumArtist text,                          \
                                                                            AlbumTrackCount integer,                   \
                                                                            AlbumTrackNumber integer,                  \
                                                                            ArtistID integer,                          \
                                                                            Artist text,                               \
                                                                            ArtworkFileID integer,                     \
                                                                            ComposerID integer,                        \
                                                                            Composer text,                             \
                                                                            GenreID integer,                           \
                                                                            Genre text,                                \
                                                                            IsBanSupported integer,                    \
                                                                            IsBanned integer,                          \
                                                                            IslikeSupported integer,                   \
                                                                            Isliked integer,                           \
                                                                            RegidentOnDevice integer,                  \
                                                                            IsPartOfCompilation integer,               \
                                                                            MediaType integer,                         \
                                                                            MediaRating integer,                       \
                                                                            MediaDurationMs integer,                   \
                                                                            ChapterCount integer);"


#define IPOD_PLAYER_IAP2_DB_CREATE_TRACKTITLE_INDEX     "CREATE INDEX TrackIndex ON MediaItem(TrackTitle);"

#define IPOD_PLAYER_IAP2_DB_CREATE_ALBUMTITLE_INDEX     "CREATE INDEX AlbumIndex ON MediaItem(AlbumTitle);"

#define IPOD_PLAYER_IAP2_DB_CREATE_ARTISTTITLE_INDEX    "CREATE INDEX ArtistIndex ON MediaItem(Artist);"

#define IPOD_PLAYER_IAP2_DB_INSERT_DEVICEID             "INSERT INTO MediaInfo(DeviceID) VALUES('%s');"

#define IPOD_PLAYER_IAP2_DB_REPLACE_MEDIAINFO           "UPDATE MediaInfo SET MediaID=?, MediaName=?, MediaType=? WHERE DeviceID=?;"

#define IPOD_PLAYER_IAP2_DB_UPDATE_REVISION             "UPDATE MediaInfo SET Revision=?, Progress=? WHERE MediaID=?;"

#define IPOD_PLAYER_IAP2_DB_PROGRESS_MEDIAID            "UPDATE MediaInfo SET Progress=? WHERE MediaID=?;"

#define IPOD_PLAYER_IAP2_DB_UPDATE_HIDING               "UPDATE MediaInfo SET IsHiding=? WHERE MediaID=?;"

#define IPOD_PLAYER_IAP2_DB_QUERY_MEDIAINFO             "SELECT * FROM MediaInfo WHERE DeviceID='%s';"

#define IPOD_PLAYER_IAP2_DB_QUERY_MEDIAINFO_ID          "SELECT MediaID FROM MediaInfo; "

#define IPOD_PLAYER_IAP2_DB_QUERY_MEDIAINFO_PROGRESS    "SELECT Progress FROM MediaInfo;"

#define IPOD_PLAYER_IAP2_DB_QUERY_MEDIAINFO_NAME        "SELECT MediaName FROM MediaInfo"

#define IPOD_PLAYER_IAP2_DB_UPDATE_MEDIAINFO_NAME       "UPDATE MediaInfo SET MediaName='%s' WHERE DeviceID='%s';"

#define IPOD_PLAYER_IAP2_DB_QUERY_DEVICEID              "SELECT DeviceID FROM MediaInfo"

#define IPOD_PLAYER_IAP2_DB_CREATE_PLAYLIST_TABLE "CREATE TABLE Playlist(   PlaylistID integer PRIMARY KEY, \
                                                                            PlaylistName text,              \
                                                                            PlaylistParentID integer,       \
                                                                            PlaylistGeniusMix integer,      \
                                                                            PlaylistFolder integer,         \
                                                                            PlaylistFileTransferID integer, \
                                                                            PlaylistAppleMusicRadio integer);"

#define IPOD_PLAYER_IAP2_DB_INSERT_PLAYLIST "INSERT OR REPLACE INTO Playlist(   PlaylistID,                 \
                                                                                PlaylistName,               \
                                                                                PlaylistParentID,           \
                                                                                PlaylistGeniusMix,          \
                                                                                PlaylistFolder,             \
                                                                                PlaylistFileTransferID,     \
                                                                                PlaylistAppleMusicRadio)    \
                                                                        VALUES(?, ?, ?, ?, ?, ?, ?);"

#define IPOD_PLAYER_IAP2_DB_CREATE_PLAYLIST_TRACKS_TABLE "CREATE TABLE PlaylistTracks(  TrackIndex integer, \
                                                                                        PlaylistID integer, \
                                                                                        TrackID integer);"

#define IPOD_PLAYER_IAP2_DB_INSERT_PLAYLIST_TRACKS "INSERT INTO PlaylistTracks(  TrackIndex,     \
                                                                                 PlaylistID,     \
                                                                                 TrackID)        \
                                                                           VALUES(?, ?, ?);"

/* ######################################################################## */


/* ###################### SQL statement for iPodInfo database #################*/
#define IPOD_PLAYER_IAP2_DB_CREATE_CONNECTIONSTATUS_TABLE "CREATE TABLE ConnectionStatus(Device integer,             \
                                                                                         Authentication integer,     \
                                                                                         Power integer );"

#define IPOD_PLAYER_IAP2_DB_CREATE_IPODINFO_TABLE "CREATE TABLE iPodInfo(   iPodID text,              \
                                                                            PlaybackStatus integer,   \
                                                                            Speed integer,            \
                                                                            Shuffle integer,          \
                                                                            Repeat integer,           \
                                                                            SampleRate integer,       \
                                                                            AppName text,             \
                                                                            AppBundleID text,         \
                                                                            ElapsedTime integer,      \
                                                                            MediaLibraryUID text,     \
                                                                            RadioAd integer,          \
                                                                            RadioStationName text,    \
                                                                            RadioStationID integer,   \
                                                                            ChapterIndex integer,     \
                                                                            TrackIndex integer,       \
                                                                            QueueCount integer,       \
                                                                            QueueListAvail integer);"

#define IPOD_PLAYER_IAP2_DB_CREATE_CATEGORY_TABLE "CREATE TABLE CategoryList(CatIndex integer, Category text PRIMARY KEY, CatValue integer);"

#define IPOD_PLAYER_IAP2_DB_CREATE_PLAYING_ITEM_TABLE "CREATE TABLE PlayingItem(iPodID integer PRIMARY KEY,            \
                                                                                TrackID integer,                       \
                                                                                TrackTitle text,                       \
                                                                                AlbumID integer,                       \
                                                                                AlbumTitle text,                       \
                                                                                AlbumDiscCount integer,                \
                                                                                AlbumDiscNumber integer,               \
                                                                                AlbumArtistID integer,                 \
                                                                                AlbumArtist text,                      \
                                                                                AlbumTrackCount integer,               \
                                                                                AlbumTrackNumber integer,              \
                                                                                ArtistID integer,                      \
                                                                                Artist text,                           \
                                                                                ArtworkFileID integer,                 \
                                                                                ComposerID integer,                    \
                                                                                Composer text,                         \
                                                                                GenreID integer,                       \
                                                                                Genre text,                            \
                                                                                IsBanSupported integer,                \
                                                                                IsBanned integer,                      \
                                                                                IslikeSupported integer,               \
                                                                                Isliked integer,                       \
                                                                                RegidentOnDevice integer,              \
                                                                                IsPartOfCompilation integer,           \
                                                                                MediaType integer,                     \
                                                                                MediaRating integer,                   \
                                                                                MediaDurationMs integer,               \
                                                                                ChapterCount integer);"
                                                                                
#define IPOD_PLAYER_IAP2_DB_CREATE_BT_STATUS_TABLE "CREATE TABLE BluetoothStatus(BtID integer, Profile integer);"
#define IPOD_PLAYER_IAP2_DB_CREATE_ASSISTIVE_TABLE "CREATE TABLE AssistiveStatus(AssistiveID integer, Status integer);"



#define IPOD_PLAYER_IAP2_DB_INSERT_ITEM_PREPARE "INSERT OR REPLACE INTO MediaItem( TrackID,                            \
                                                                                   TrackTitle,                         \
                                                                                   AlbumID,                            \
                                                                                   AlbumTitle,                         \
                                                                                   AlbumDiscCount,                     \
                                                                                   AlbumDiscNumber,                    \
                                                                                   AlbumArtistID,                      \
                                                                                   AlbumArtist,                        \
                                                                                   AlbumTrackCount,                    \
                                                                                   AlbumTrackNumber,                   \
                                                                                   ArtistID,                           \
                                                                                   Artist,                             \
                                                                                   ArtworkFileID,                      \
                                                                                   ComposerID,                         \
                                                                                   Composer,                           \
                                                                                   GenreID,                            \
                                                                                   Genre,                              \
                                                                                   IsBanSupported,                     \
                                                                                   IsBanned,                           \
                                                                                   IslikeSupported,                    \
                                                                                   Isliked,                            \
                                                                                   RegidentOnDevice,                   \
                                                                                   IsPartOfCompilation,                \
                                                                                   MediaType,                          \
                                                                                   MediaRating,                        \
                                                                                   MediaDurationMs,                    \
                                                                                   ChapterCount)                       \
                                                                VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"


#define IPOD_PLAYER_IAP2_DB_PLAYING_ITEM_PREPARE "INSERT OR REPLACE INTO PlayingItem(  iPodID,                         \
                                                                            TrackID,                                   \
                                                                            TrackTitle,                                \
                                                                            AlbumID,                                   \
                                                                            AlbumTitle,                                \
                                                                            AlbumDiscCount,                            \
                                                                            AlbumDiscNumber,                           \
                                                                            AlbumArtistID,                             \
                                                                            AlbumArtist,                               \
                                                                            AlbumTrackCount,                           \
                                                                            AlbumTrackNumber,                          \
                                                                            ArtistID,                                  \
                                                                            Artist,                                    \
                                                                            ArtworkFileID,                             \
                                                                            ComposerID,                                \
                                                                            Composer,                                  \
                                                                            GenreID,                                   \
                                                                            Genre,                                     \
                                                                            IsBanSupported,                            \
                                                                            IsBanned,                                  \
                                                                            IslikeSupported,                           \
                                                                            Isliked,                                   \
                                                                            RegidentOnDevice,                          \
                                                                            IsPartOfCompilation,                       \
                                                                            MediaType,                                 \
                                                                            MediaRating,                               \
                                                                            MediaDurationMs,                           \
                                                                            ChapterCount)                              \
                                                 VALUES(1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"

#define IPOD_PLAYER_IAP2_DB_IPODINFO_ITEM_PREPARE "INSERT OR REPLACE INTO iPodInfo( iPodID,              \
                                                                                    PlaybackStatus,      \
                                                                                    Speed,               \
                                                                                    Shuffle,             \
                                                                                    Repeat,              \
                                                                                    AppName,             \
                                                                                    AppBundleID,         \
                                                                                    ElapsedTime,         \
                                                                                    MediaLibraryUID,     \
                                                                                    RadioAd,             \
                                                                                    RadioStationName,    \
                                                                                    RadioStationID,      \
                                                                                    ChapterIndex,        \
                                                                                    TrackIndex,          \
                                                                                    QueueCount,          \
                                                                                    QueueListAvail)      \
                                                            VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"

#define IPOD_PLAYER_IAP2_DB_ASSISTIVE_PREPARE "INSERT OR REPLACE INTO AssistiveStatus(AssistiveID, Status) VALUES(%d, %d);"
#define IPOD_PLAYER_IAP2_DB_BT_STATUS_PREPARE "INSERT OR REPLACE INTO BluetoothStatus(BtID, Profile) VALUES(%d, %d);"

#define IPOD_PLAYER_IAP2_DB_CREATE_NOWPLAYING_TABLE "CREATE TABLE NowPlaying(TrackIndex integer PRIMARY KEY, TrackID integer, TrackName text);"

#define IPOD_PLAYER_IAP2_DB_QUERY_COUNT_NOWPLAYING "SELECT COUNT(TrackIndex) FROM NowPlaying "
#define IPOD_PLAYER_IAP2_DB_QUERY_LIST_NOWPLAYING "SELECT TrackIndex, TrackID, TrackName FROM NowPlaying "
#define IPOD_PLAYER_IAP2_DB_QUERY_TRACKINFO_FROM_PLAYINGITEM "SELECT TrackTitle, AlbumTitle, Artist, Composer, Genre, MediaDurationMs, ChapterCount, TrackID FROM PlayingItem "


#define IPOD_PLAYER_IAP2_DB_IPODINFO_UPDATE_PREPARE_START "UPDATE iPodInfo SET "
#define IPOD_PLAYER_IAP2_DB_IPODINFO_UPDATE_PREPARE_VALUES "') VALUES("
#define IPOD_PLAYER_IAP2_DB_IPODINFO_UPDATE_PREPARE_END ";"

#define IPOD_PLAYER_IAP2_DB_UPDATE_PLAYING_ITEM_PREPARE_START "UPDATE PlayingItem SET "
#define IPOD_PLAYER_IAP2_DB_UPDATE_PLAYING_ITEM_PREPARE_PARAM "=?;"

#define IPOD_PLAYER_IAP2_DB_MEDIAINFO_COLUMN_DEVICEID "DeviceID"
#define IPOD_PLAYER_IAP2_DB_MEDIAINFO_COLUMN_MEDIAID "MediaID"
#define IPOD_PLAYER_IAP2_DB_MEDIAINFO_COLUMN_MEDIANAME "MediaName"
#define IPOD_PLAYER_IAP2_DB_MEDIAINFO_COLUMN_MEDIATYPE "MediaType"
#define IPOD_PLAYER_IAP2_DB_MEDIAINFO_COLUMN_REVISION "Revision"
/* ######################################################################## */

/* ################# SQL statement for Category List #################### */
#define IPOD_PLAYER_IAP2_DB_QUERY_LIST_SELECT_ALL               "SELECT DISTINCT TrackID, TrackTitle FROM MediaItem "
#define IPOD_PLAYER_IAP2_DB_QUERY_LIST_SELECT_PLAYLIST          "SELECT DISTINCT PlaylistID, PlaylistName FROM Playlist "
#define IPOD_PLAYER_IAP2_DB_QUERY_LIST_SELECT_ARTIST            "SELECT DISTINCT ArtistID, Artist    FROM MediaItem WHERE MediaType=0 "
#define IPOD_PLAYER_IAP2_DB_QUERY_LIST_SELECT_ALBUM             "SELECT DISTINCT AlbumID, AlbumTitle FROM MediaItem WHERE MediaType=0 "
#define IPOD_PLAYER_IAP2_DB_QUERY_LIST_SELECT_GENRE             "SELECT DISTINCT GenreID, Genre      FROM MediaItem WHERE MediaType=0 "
#define IPOD_PLAYER_IAP2_DB_QUERY_LIST_SELECT_TRACK             "SELECT DISTINCT TrackID, TrackTitle FROM MediaItem "
#define IPOD_PLAYER_IAP2_DB_QUERY_LIST_SELECT_TRACK_FROM_PLAYLIST "SELECT DISTINCT MediaItem.TrackID, MediaItem.TrackTitle, TrackIndex FROM MediaItem LEFT OUTER JOIN PlaylistTracks ON  PlaylistTracks.TrackID=MediaItem.TrackID "
#define IPOD_PLAYER_IAP2_DB_QUERY_LIST_SELECT_COMPOSER          "SELECT DISTINCT ComposerID, Composer FROM MediaItem WHERE MediaType=0 "
#define IPOD_PLAYER_IAP2_DB_QUERY_LIST_SELECT_AUDIOBOOK         "SELECT DISTINCT TrackID, TrackTitle FROM MediaItem WHERE MediaType=2 ORDER BY TrackTitle ASC, TrackID ASC  "
/* Podcast uses AlbumID and AlbumTitle in GetDBEntry. Podcast selects album(program?) in first time. */
#define IPOD_PLAYER_IAP2_DB_QUERY_LIST_SELECT_PODCAST           "SELECT DISTINCT AlbumID, AlbumTitle FROM MediaItem " 
#define IPOD_PLAYER_IAP2_DB_QUERY_LIST_SELECT_NESTED_PLAYLIST   "SELECT DISTINCT TrackID, TrackTitle FROM MediaItem  ORDER BY TrackTitle ASC "
#define IPOD_PLAYER_IAP2_DB_QUERY_LIST_SELECT_ITUNESU           "SELECT DISTINCT TrackID, TrackTitle FROM MediaItem WHERE MediaType=3 ORDER BY TrackTitle ASC, TrackID ASC  "

#define IPOD_PLAYER_IAP2_DB_ORDER_TRACKID       "ORDER BY TrackID ASC "
#define IPOD_PLAYER_IAP2_DB_ORDER_TRACKINDEX    "ORDER BY TrackIndex ASC "
#define IPOD_PLAYER_IAP2_DB_ORDER_ARTISTID      "ORDER BY ArtistID ASC "
#define IPOD_PLAYER_IAP2_DB_ORDER_ALBUMID       "ORDER BY AlbumID ASC "
#define IPOD_PLAYER_IAP2_DB_ORDER_GENREID       "ORDER BY GenreID ASC "
#define IPOD_PLAYER_IAP2_DB_ORDER_COMPOSERID    "ORDER BY ComposerID ASC "

#define IPOD_PLAYER_IAP2_DB_ORDER_PLAYLISTID_TRACKINDEX    "ORDER BY TrackIndex ASC, MediaItem.TrackTitle ASC, PlaylistID ASC "
#define IPOD_PLAYER_IAP2_DB_ORDER_ARTISTID_ALBUMID  "ORDER BY ArtistID ASC, AlbumID ASC "
#define IPOD_PLAYER_IAP2_DB_ORDER_ARTISTID_ALBUMID_ALBUMTRACKCOUNT_TRACKID  "ORDER BY ArtistID ASC, AlbumID ASC, AlbumTrackNumber ASC, TrackID ASC "

/* ###################################################################### */



/* ################ SQL statement for Get Category Count ################# */

#define IPOD_PLAYER_IAP2_DB_QUERY_DBLIST_COUNT_TRACK_FROM_PLAYLIST "SELECT COUNT(DISTINCT TrackIndex) FROM PlaylistTracks LEFT OUTER JOIN MediaItem ON PlaylistTracks.TrackID=MediaItem.TrackID "
#define IPOD_PLAYER_IAP2_DB_QUERY_DBLIST_COUNT_PLAYLIST_FROM_PLAYLIST "SELECT COUNT(DISTINCT PlaylistID) FROM Playlist "
#define IPOD_PLAYER_IAP2_DB_SELECT_COUNT                          "SELECT COUNT(DISTINCT %s) FROM %s "

/* ###################################################################### */

/* ################# SQL statement for Get Category ID ################### */
#define IPOD_PLAYER_IAP2_DB_QUERY_CAT_PLAYLISTID        "SELECT DISTINCT PlaylistID, PlaylistName FROM Playlist "
#define IPOD_PLAYER_IAP2_DB_QUERY_CAT_ARTISTID          "SELECT DISTINCT ArtistID, Artist FROM MediaItem WHERE MediaType=0 "
#define IPOD_PLAYER_IAP2_DB_QUERY_CAT_ALBUMID           "SELECT DISTINCT AlbumID, AlbumTitle FROM MediaItem WHERE MediaType=0 "
#define IPOD_PLAYER_IAP2_DB_QUERY_CAT_GENREID           "SELECT DISTINCT GenreID, Genre FROM MediaItem WHERE MediaType=0 "
#define IPOD_PLAYER_IAP2_DB_QUERY_CAT_TRACKID           "SELECT DISTINCT TrackID, TrackTitle FROM MediaItem WHERE MediaType=0 "
#define IPOD_PLAYER_IAP2_DB_QUERY_CAT_TRACKID_FROM_PLAYLIST "SELECT DISTINCT MediaItem.TrackID, MediaItem.TrackTitle FROM MediaItem LEFT OUTER JOIN PlaylistTracks ON PlaylistTracks.TrackID=MediaItem.TrackID "
#define IPOD_PLAYER_IAP2_DB_QUERY_CAT_COMPOSERID        "SELECT DISTINCT ComposerID, Composer FROM MediaItem WHERE MediaType=0 "
#define IPOD_PLAYER_IAP2_DB_QUERY_CAT_AUDIOBOOKID       "SELECT DISTINCT TrackID, TrackTitle FROM MediaItem WHERE MediaType=2 "
#define IPOD_PLAYER_IAP2_DB_QUERY_CAT_PODCASTID         "SELECT DISTINCT AlbumID, AlbumTitle FROM MediaItem WHERE MediaType=1 "
#define IPOD_PLAYER_IAP2_DB_QUERY_CAT_NESTED_PLAYLISTID "SELECT DISTINCT TrackID, TrackTitle FROM MediaItem "
#define IPOD_PLAYER_IAP2_DB_QUERY_CAT_ITUNESUID         "SELECT DISTINCT TrackID, TrackTitle FROM MediaItem WHERE MediaType=3 "

#define IPOD_PLAYER_IAP2_DB_ORDER_PLAYLIST              "ORDER BY PlaylistName COLLATE nocase ASC, PlaylistID ASC "
#define IPOD_PLAYER_IAP2_DB_ORDER_ALBUM                 "ORDER BY AlbumTitle COLLATE nocase ASC, AlbumID ASC "
#define IPOD_PLAYER_IAP2_DB_ORDER_ALBUMTRACKCOUNT       "ORDER BY AlbumTrackNumber COLLATE nocase ASC, TrackID ASC "
#define IPOD_PLAYER_IAP2_DB_ORDER_ALBUMTRACKCOUNT_TRACK       "ORDER BY AlbumTrackNumber ASC, TrackTitle ASC, TrackID ASC "
#define IPOD_PLAYER_IAP2_DB_ORDER_ARTIST                "ORDER BY Artist COLLATE nocase ASC, ArtistID ASC  "
#define IPOD_PLAYER_IAP2_DB_ORDER_GENRE                 "ORDER BY Genre COLLATE nocase ASC, GenreID ASC "
#define IPOD_PLAYER_IAP2_DB_ORDER_TRACK                 "ORDER BY TrackTitle COLLATE nocase ASC, TrackID ASC "
#define IPOD_PLAYER_IAP2_DB_ORDER_COMPOSER              "ORDER BY Composer COLLATE nocase ASC, ComposerID ASC "

/* ####################################################################### */




/* ################# SQL statement for Set Category ID ################### */
#define IPOD_PLAYER_IAP2_DB_QUERY_CAT_COUNT "SELECT COUNT(CatIndex) FROM CategoryList;"

#define IPOD_PLAYER_IAP2_DB_INSERT_CAT_PLAYLISTID   "INSERT INTO CategoryList(CatIndex, Category, CatValue) VALUES(%d, 'PlaylistID', %lld);"
#define IPOD_PLAYER_IAP2_DB_INSERT_CAT_ARTISTID     "INSERT INTO CategoryList(CatIndex, Category, CatValue) VALUES(%d, 'ArtistID', %lld);"
#define IPOD_PLAYER_IAP2_DB_INSERT_CAT_ALBUMID      "INSERT INTO CategoryList(CatIndex, Category, CatValue) VALUES(%d, 'AlbumID', %lld);"
#define IPOD_PLAYER_IAP2_DB_INSERT_CAT_GENREID      "INSERT INTO CategoryList(CatIndex, Category, CatValue) VALUES(%d, 'GenreID', %lld);"
#define IPOD_PLAYER_IAP2_DB_INSERT_CAT_TRACKID      "INSERT INTO CategoryList(CatIndex, Category, CatValue) VALUES(%d, 'TrackID', %lld);"
#define IPOD_PLAYER_IAP2_DB_INSERT_CAT_COMPOSERID   "INSERT INTO CategoryList(CatIndex, Category, CatValue) VALUES(%d, 'ComposerID', %lld);"
#define IPOD_PLAYER_IAP2_DB_INSERT_CAT_NESTEDID     "INSERT INTO CategoryList(CatIndex, Category, CatValue) VALUES(%d, 'NestedPlaylistID', %lld);"

#define IPOD_PLAYER_IAP2_DB_INSERT_CAT_AUDIOBOOKID  "INSERT INTO CategoryList(CatIndex, Category, CatValue) VALUES(%d, 'MediaType=2 AND TrackID', %lld);"
#define IPOD_PLAYER_IAP2_DB_INSERT_CAT_PODCASTID    "INSERT INTO CategoryList(CatIndex, Category, CatValue) VALUES(%d, 'MediaType=1 AND AlbumID', %lld);"
#define IPOD_PLAYER_IAP2_DB_INSERT_CAT_ITUNESUID    "INSERT INTO CategoryList(CatIndex, Category, CatValue) VALUES(%d, 'MediaType=3 AND TrackID', %lld);"

/* ####################################################################### */

/* ############## SQL statement for Clear selecting Category ############# */
#define IPOD_PLAYER_IAP2_DB_DELETE_CAT_ALL          "DELETE FROM CategoryList "
#define IPOD_PLAYER_IAP2_DB_DELETE_CAT_PLAYLIST     "DELETE FROM CategoryList WHERE Category='PlaylistID'"
#define IPOD_PLAYER_IAP2_DB_DELETE_CAT_ARTIST       "DELETE FROM CategoryList WHERE Category='ArtistID'"
#define IPOD_PLAYER_IAP2_DB_DELETE_CAT_ALBUM        "DELETE FROM CategoryList WHERE Category='AlbumID'"
#define IPOD_PLAYER_IAP2_DB_DELETE_CAT_GENRE        "DELETE FROM CategoryList WHERE Category='GenreID'"
#define IPOD_PLAYER_IAP2_DB_DELETE_CAT_TRACK        "DELETE FROM CategoryList WHERE Category='TrackID'"
#define IPOD_PLAYER_IAP2_DB_DELETE_CAT_COMPOSER     "DELETE FROM CategoryList WHERE Category='ComposerID'"
#define IPOD_PLAYER_IAP2_DB_DELETE_CAT_AUDIOBOOK    "DELETE FROM CategoryList WHERE Category='MediaType=2 AND TrackID'"
#define IPOD_PLAYER_IAP2_DB_DELETE_CAT_PODCAST      "DELETE FROM CategoryList WHERE Category='MediaType=1 AND AlbumID'"
#define IPOD_PLAYER_IAP2_DB_DELETE_CAT_ITUNEU       "DELETE FROM CategoryList WHERE Category='MediaType=3 AND TrackID'"

/* ####################################################################### */

/* ################### SQL statement for Get Media Id  ################### */
#define IPOD_PLAYER_IAP2_DB_QUERY_PLAYLISTID_FROM_MEDIAITEM     "SELECT PlailistID FROM Playlist "
#define IPOD_PLAYER_IAP2_DB_QUERY_ARTISTID_FROM_MEDIAITEM       "SELECT ArtistID FROM MediaItem "
#define IPOD_PLAYER_IAP2_DB_QUERY_ALBUMID_FROM_MEDIAITEM        "SELECT AlbumID FROM MediaItem "
#define IPOD_PLAYER_IAP2_DB_QUERY_GENREID_FROM_MEDIAITEM        "SELECT GenreID FROM MediaItem "
#define IPOD_PLAYER_IAP2_DB_QUERY_TRACKID_FROM_MEDIAITEM        "SELECT TrackID FROM MediaItem "
#define IPOD_PLAYER_IAP2_DB_QUERY_TRACKID_FROM_PLAYLIST         "SELECT MediaItem.TrackID FROM MediaItem LEFT OUTER JOIN PlaylistTracks ON PlaylistTracks.TrackID=MediaItem.TrackID "
#define IPOD_PLAYER_IAP2_DB_QUERY_COMPOSERID_FROM_MEDIAITEM     "SELECT ComposerID FROM MediaItem "
#define IPOD_PLAYER_IAP2_DB_QUERY_TRACKINFO_FROM_MEDIAITEM      "SELECT TrackTitle, AlbumTitle, Artist, Composer, Genre, MediaDurationMs, ChapterCount, TrackID, MediaType FROM MediaItem "
#define IPOD_PLAYER_IAP2_DB_QUERY_MEDIATYPE_FROM_MEDIAITEM      "SELECT MediaType FROM MediaItem "

#define IPOD_PLAYER_IAP2_DB_QUERY_PLAYLISTID_FROM_NOWPLAYING    "SELECT TrackID FROM NowPlaying "
#define IPOD_PLAYER_IAP2_DB_QUERY_ARTISTID_FROM_NOWPLAYING      "SELECT ArtistID FROM NowPlaying "
#define IPOD_PLAYER_IAP2_DB_QUERY_ALBUMID_FROM_NOWPLAYING       "SELECT AlbumID FROM NowPlaying "
#define IPOD_PLAYER_IAP2_DB_QUERY_GENREID_FROM_NOWPLAYING       "SELECT GenreID FROM NowPlaying "
#define IPOD_PLAYER_IAP2_DB_QUERY_TRACKID_FROM_NOWPLAYING       "SELECT TrackID FROM NowPlaying "
#define IPOD_PLAYER_IAP2_DB_QUERY_COMPOSERID_FROM_NOWPLAYING    "SELECT ComposerID FROM NowPlaying "


/* ####################################################################### */
#define IPOD_PLAYER_IAP2_DB_COLUMN_TRACKID              "TrackID"
#define IPOD_PLAYER_IAP2_DB_COLUMN_TRACKTITLE           "TrackTitle"
#define IPOD_PLAYER_IAP2_DB_COLUMN_ALBUMID              "AlbumID"
#define IPOD_PLAYER_IAP2_DB_COLUMN_ALBUMTITLE           "AlbumTitle"
#define IPOD_PLAYER_IAP2_DB_COLUMN_ALBUMDISCCOUNT       "AlbumDiscCount"
#define IPOD_PLAYER_IAP2_DB_COLUMN_ALBUMDISCNUMBER      "AlbumDiscNumber"
#define IPOD_PLAYER_IAP2_DB_COLUMN_ALBUMARTISTID        "AlbumArtistID"
#define IPOD_PLAYER_IAP2_DB_COLUMN_ALBUMARTIST          "AlbumArtist"
#define IPOD_PLAYER_IAP2_DB_COLUMN_ALBUMTRACKCOUNT      "AlbumTrackCount"
#define IPOD_PLAYER_IAP2_DB_COLUMN_ALBUMTRACKNUMBER     "AlbumTrackNumber"
#define IPOD_PLAYER_IAP2_DB_COLUMN_ARTISTID             "ArtistID"
#define IPOD_PLAYER_IAP2_DB_COLUMN_ARTIST               "Artist"
#define IPOD_PLAYER_IAP2_DB_COLUMN_ARTWORKFILEID        "ArtworkFileID"
#define IPOD_PLAYER_IAP2_DB_COLUMN_COMPOSERID           "ComposerID"
#define IPOD_PLAYER_IAP2_DB_COLUMN_COMPOSER             "Composer"
#define IPOD_PLAYER_IAP2_DB_COLUMN_GENREID              "GenreID"
#define IPOD_PLAYER_IAP2_DB_COLUMN_GENRE                "Genre"
#define IPOD_PLAYER_IAP2_DB_COLUMN_ISBANSUPPORTED       "IsBanSupported"
#define IPOD_PLAYER_IAP2_DB_COLUMN_ISBANNED             "IsBanned"
#define IPOD_PLAYER_IAP2_DB_COLUMN_ISLIKESUPPORTED      "IslikeSupported"
#define IPOD_PLAYER_IAP2_DB_COLUMN_ISLIKED              "Isliked"
#define IPOD_PLAYER_IAP2_DB_COLUMN_REGIDENTONDEVICE     "RegidentOnDevice"
#define IPOD_PLAYER_IAP2_DB_COLUMN_ISPARTOFCOMPILATION  "IsPartOfCompilation"
#define IPOD_PLAYER_IAP2_DB_COLUMN_MEDIATYPE            "MediaType"
#define IPOD_PLAYER_IAP2_DB_COLUMN_MEDIARATING          "MediaRating"
#define IPOD_PLAYER_IAP2_DB_COLUMN_MEDIADURATIONMS      "MediaDurationMs"
#define IPOD_PLAYER_IAP2_DB_COLUMN_PLAYLIST             "PlaylistID"
#define IPOD_PLAYER_IAP2_DB_COLUMN_CHAPTERCOUNT         "ChapterCount"


/* ############### SQL statement for Set NowPlaying MediaIDs ############# */
#define IPOD_PLAYER_IAP2_DB_DELETE_NOWPLAYING "DELETE FROM NowPlaying"
#define IPOD_PLAYER_IAP2_DB_INSERT_NOWPLAYING "INSERT INTO NowPlaying(TrackIndex, TrackID) VALUES(?, ?);"

/* ####################################################################### */

#define IPOD_PLAYER_IAP2_DB_QUERY_LIST_WHERE "WHERE %s=%lld "
#define IPOD_PLAYER_IAP2_DB_QUERY_LIST_AND "AND %s=%lld "
#define IPOD_PLAYER_IAP2_DB_QUERY_LIST_OR "OR %s=%lld "
#define IPOD_PLAYER_IAP2_DB_QUERY_LIST_LIMIT_OFFSET "limit %d offset %lld "

#define IPOD_PLAYER_IAP2_DB_QUERY_LIST_WHERE_32 "WHERE %s=%d "
#define IPOD_PLAYER_IAP2_DB_QUERY_LIST_AND_32 "AND %s=%d "

/* ################### SQL statement for Get Device Info  ################### */
#define IPOD_PLAYER_IAP2_DB_QUERY_SHUFFLE_STATUS "SELECT Shuffle FROM iPodInfo"
#define IPOD_PLAYER_IAP2_DB_QUERY_REPEAT_STATUS "SELECT Repeat FROM iPodInfo"
#define IPOD_PLAYER_IAP2_DB_QUERY_QUEUE_COUNT "SELECT QueueCount FROM iPodInfo"
#define IPOD_PLAYER_IAP2_DB_QUERY_IOSAPP_NAME "SELECT AppName FROM iPodInfo"
/* ########################################################################## */

/* ################### SQL statement for Delete Items  ################### */
#define IPOD_PLAYER_IAP2_DB_DELETE_MEDIA_ITEM           "DELETE FROM MediaItem WHERE TrackID=?;"
#define IPOD_PLAYER_IAP2_DB_DELETE_PLAYLIST             "DELETE FROM Playlist WHERE PlaylistID=?;"
#define IPOD_PLAYER_IAP2_DB_DELETE_PLAYLIST_TRACKS      "DELETE FROM PlaylistTracks WHERE PlaylistID=?;"
#define IPOD_PLAYER_IAP2_DB_DELETE_ALL_MEDIA            "DELETE FROM MediaItem"
#define IPOD_PLAYER_IAP2_DB_DELETE_ALL_PLAYLIST         "DELETE FROM Playlist"
#define IPOD_PLAYER_IAP2_DB_DELETE_ALL_PLAYLIST_TRACKS  "DELETE FROM PlaylistTracks"

/* ########################################################################## */
#define IPOD_PLAYER_IAP2_DB_COLUMN_IPODID   "iPodID"
#define IPOD_PLAYER_IAP2_DB_COLUMN_PLAYBACKSTATUS   "PlaybackStatus"
#define IPOD_PLAYER_IAP2_DB_COLUMN_SPEED "Speed"
#define IPOD_PLAYER_IAP2_DB_COLUMN_SHUFFLE "Shuffle"
#define IPOD_PLAYER_IAP2_DB_COLUMN_REPEAT "Repeat"
#define IPOD_PLAYER_IAP2_DB_COLUMN_APPNAME "AppName"
#define IPOD_PLAYER_IAP2_DB_COLUMN_APPBUNDLEID "AppBundleID"
#define IPOD_PLAYER_IAP2_DB_COLUMN_ELAPSEDTIME "ElapsedTime"
#define IPOD_PLAYER_IAP2_DB_COLUMN_MEDIALIBRARYUID "MediaLibraryUID"
#define IPOD_PLAYER_IAP2_DB_COLUMN_RADIOAD "RadioAd"
#define IPOD_PLAYER_IAP2_DB_COLUMN_RADIOSTATIONNAME "RadioStationName"
#define IPOD_PLAYER_IAP2_DB_COLUMN_RADIOSTATIONID "RadioStationID"
#define IPOD_PLAYER_IAP2_DB_COLUMN_CHAPTERINDEX "ChapterIndex"
#define IPOD_PLAYER_IAP2_DB_COLUMN_INDEX "TrackIndex"
#define IPOD_PLAYER_IAP2_DB_COLUMN_QUEUECOUNT "QueueCount"
#define IPOD_PLAYER_IAP2_DB_COLUMN_QUEUELISTAVAIL "QueueListAvail"

#define IPOD_PLAYER_IAP2_DB_QUERY_BT_STATUS "SELECT Profile FROM BluetoothStatus WHERE BtID=%d;"
#define IPOD_PLAYER_IAP2_DB_QUERY_ASSISTIVE_STATUS "SELECT Status FROM AssistiveStatus WHERE AssistiveID=%d;"

#define IPOD_PLAYER_IAP2_DB_QUERY_TABLE_COUNT "SELECT COUNT(*)  FROM sqlite_master WHERE type='table' and name='MediaItem';"

#define IPOD_PLAYER_IAP2_DB_QUERY_COUNT_PREPARE "SELECT COUNT(*)  FROM %s WHERE %s=? "

#define IPOD_PLAYER_IAP2_DB_QUERY_SELECT_COUNT "SELECT COUNT(*) FROM %s "

#define IPOD_PLAYER_IAP2_DB_QUERY_CAT_LIST "SELECT CatIndex, Category, CatValue from CategoryList"

#define IPOD_PLAYER_IAP2_DB_COLUMN_CAT_INDEX "CatIndex"
#define IPOD_PLAYER_IAP2_DB_COLUMN_CAT_CATEGORY "Category"
#define IPOD_PLAYER_IAP2_DB_COLUMN_CAT_VALUE "CatValue"
#define IPOD_PLAYER_IAP2_DB_TABLE_CATEGORY "CategoryList"

#define IPOD_PLAYER_IAP2_DB_TABLE_NOWPLAYING "NowPlaying"

#define IPOD_PLAYER_IAP2_DB_QUERY_SELECT "SELECT "
#define IPOD_PLAYER_IAP2_DB_QUERY_COUNT "COUNT "
#define IPOD_PLAYER_IAP2_DB_QUERY_WHERE "WHERE "

#define IPOD_PLAYER_IAP2_DB_QUERY_PLAYBACK_STATUS   "SELECT PlaybackStatus, TrackIndex, ChapterIndex, ElapsedTime, AppName, AppBundleID, MediaLibraryUID, RadioStationName, QueueCount, QueueListAvail FROM iPodInfo"
#define IPOD_PLAYER_IAP2_DB_QUERY_SAMPLE_RATE       "SELECT SampleRate FROM iPodInfo"
#define IPOD_PLAYER_IAP2_DB_UPDATE_SAMPLE_RATE      "UPDATE iPodInfo SET SampleRate=%d;"

#define IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_INT -1
#define IPOD_PLAYER_IAP2_DB_MEDIA_ITEM_EMPTY_TXT ""

#define IPOD_PLAYER_IAP2_DB_PREPARE_PARAM "? "
#define IPOD_PLAYER_IAP2_DB_PREPARE_EQUALE "="

#define IPOD_PLAYER_IAP2_DB_PREPARE_PARAM_COMMA "?, "
#define IPOD_PLAYER_IAP2_DB_PREPARE_COMMA ","
#define IPOD_PLAYER_IAP2_DB_PREPARE_COMMA_SPAGE ", "
#define IPOD_PLAYER_IAP2_IN_MEMORY_DB ":memory:"
#define IPOD_PLAYER_IAP2_DB_MAIN "main"

#define IPOD_PLAYER_IAP2_DB_TRANSACTION_BEGIN "BEGIN"
#define IPOD_PLAYER_IAP2_DB_TRANSACTION_COMMIT "COMMIT"

#define IPOD_PLAYER_IAP2_DB_NULL_LEN 1

#define IPP_IAP2_UNIQUE_ID_MAX      256     /* max length of unique id strings */

#define IPOD_PLAYER_IAP2_DB_U32_MAX_DIGIT 10
#define IPOD_PLAYER_IAP2_DB_U64_MAX_DIGIT 20

typedef struct
{
    U8      id[IPP_IAP2_UNIQUE_ID_MAX];
    U32     len;
} UniqueId_t, *PUniqueId_t;

typedef struct
{
    U8 mediaID[256];
    U8 mediaName[256];
    U32 mediaType;
    U8 mediaRevision[256];
} IPOD_PLAYER_IAP2_DB_MEDIAINFO;

typedef struct
{
    U8 category[256];
    U64 catID;
} IPOD_PLAYER_IAP2_DB_CATEGORY;
    
typedef struct
{
    U32 count;
    IPOD_PLAYER_IAP2_DB_CATEGORY *categories;
} IPOD_PLAYER_IAP2_DB_CATLIST;

typedef struct
{
    U16 count;
    U64 *mediaId;
} IPOD_PLAYER_IAP2_DB_IDLIST;

typedef struct
{
    U32 count;
    IPOD_PLAYER_TRACK_INFO *trackInfo;
} IPOD_PLAYER_IAP2_DB_TRACKLIST;


typedef struct
{
    U8 *str;
    U32 len;
} ippiAP2DBString_t;

typedef struct
{
    U16 id;
    U16 profile;
} IAP2_BLUETOOTH_INFO;

typedef enum
{
    IPP_IAP2_DB_BIND_U8 = 0x00,
    IPP_IAP2_DB_BIND_U16,
    IPP_IAP2_DB_BIND_U32,
    IPP_IAP2_DB_BIND_U64,
    IPP_IAP2_DB_BIND_STR
} IPP_IAP2_DB_BIND_TYPE;

S32 ippiAP2ProgressCheckDBUpdate(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary);
void* ippiAP2CreateDB(const U8 *name, const U8 *deviceID, BOOL bluetoothConnection);
void ippiAP2CloseDB(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);
S32 ippiAP2DBSetMediaLibraryInformation(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, const U8 *key, iAP2MediaLibraryInformationSubParameter *media);
S32 ippiAP2DBGetMediaLibraryInformation(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, const U8 *key, IPOD_PLAYER_IAP2_DB_MEDIAINFO *mediaInfo);
S32 ippiAP2DBSetMediaLibraryRevision(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, const U8 *mediaID, const U8 *revision, U32 progress);
S32 ippiAP2SetMediaItemDB(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, U32 count, iAP2MediaItem *items);
S32 ippiAP2SetNowPlayingDB(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, iAP2MediaItem *item);
S32 ippiAP2SetiPodInfoDB(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, const U8 *deviceID, iAP2PlaybackAttributes *item);
S32 ippiAP2DBGetPlaybackStatus(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U32 dataSize, U8 *data);
S32 ippiAP2DBGetMediaItem(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, IPOD_PLAYER_DB_TYPE type, U32 start, U32 count, IPOD_PLAYER_IAP2_DB_CATLIST *catList, IPOD_PLAYER_ENTRY_LIST *entryList);
S32 ippiAP2DBGetCategoryCount(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, IPOD_PLAYER_TRACK_TYPE trackType, IPOD_PLAYER_DB_TYPE dbType, IPOD_PLAYER_IAP2_DB_CATLIST *catList);
S32 ippiAP2DBGetCategoryID(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, IPOD_PLAYER_DB_TYPE type, U32 catIndex, U64 *id, IPOD_PLAYER_IAP2_DB_CATLIST *catList);
S32 ippiAP2DBSetCategoryID(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IPOD_PLAYER_DB_TYPE type, U64 id);
S32 ippiAP2DBGetSelectingCategoryCount(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U32 *count);
S32 ippiAP2DBGetSelectingCategoryList(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IPOD_PLAYER_IAP2_DB_CATLIST *catList);
S32 ippiAP2DBClearSelectingCategory(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IPOD_PLAYER_DB_TYPE type);
S32 ippiAP2DBGetMediaItemID(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, IPOD_PLAYER_TRACK_TYPE trackType, IPOD_PLAYER_DB_TYPE dbType, IPOD_PLAYER_IAP2_DB_CATLIST *catList, IPOD_PLAYER_IAP2_DB_IDLIST *idList);
S32 ippiAP2DBSetNowPlayingItemID(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IPOD_PLAYER_IAP2_DB_IDLIST *idList);
S32 ippiAP2DBGetNowPlayingCount(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U32 *count);
S32 ippiAP2DBGetMediaLibraryID(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, PUniqueId_t id);
S32 ippiAP2DBGetProgress(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, U32 *progress);
S32 ippiAP2DBGetShuffle(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IPOD_PLAYER_SHUFFLE_STATUS* shuffle);
S32 ippiAP2DBGetRepeat(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IPOD_PLAYER_REPEAT_STATUS* repeat);
S32 ippiAP2DBDeleteMediaItem(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, U16 count, U64 *trackId);
S32 ippiAP2DBSetPlaylist(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, U16 count, iAP2MediaPlayList *playlist);
S32 ippiAP2DBSetIsHiding(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, const U8 *mediaID, U8 isHiding);
S32 ippiAP2DBDeletePlaylist(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, U16 count, U64 *playlistId);
S32 ippiAP2DBDeleteAllItems(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary);
S32 ippiAP2DBDeleteSelect(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IPOD_PLAYER_DB_TYPE type);
S32 ippiAP2DBGetQueueCount(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U32 *count);
S32 ippiAP2DBSetPlaylistTracks(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, U64 playlistId, U16 count, U64 *trackId);
S32 ippiAP2DBSetSample(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U32 rate);
S32 ippiAP2DBGetSample(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U32 *rate);
S32 ippiAP2DBSetDeviceName(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, const U8 *key, const U8 *name);
S32 ippiAP2DBGetDeviceName(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, U8 *name);
S32 ippiAP2DBSetAssistiveStatus(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U32 assistiveID, IPOD_PLAYER_DEVICE_EVENT_ASSISTIVE_STATUS assistiveStatus);
S32 ippiAP2DBGetAssistiveStatus(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U32 assistiveID, IPOD_PLAYER_DEVICE_EVENT_ASSISTIVE_STATUS *assistiveStatus);
S32 ippiAP2DBGetiOSAppName(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U32 length, U8 *name);
S32 ippiAP2DBGetTrackInfoList(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, IPOD_PLAYER_TRACK_TYPE type, U32 trackInfoMask, U64 trackIndex, U32 count, IPOD_PLAYER_IAP2_DB_TRACKLIST *trackList, U32 mediaType);
S32 ippiAP2DBGetNowPlayingUpdateTrackInfo(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U32 trackInfoMask, IPOD_PLAYER_TRACK_INFO *trackInfo);
S32 ippiAP2DBGetTrackTotalCount(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, IPOD_PLAYER_TRACK_TYPE type, U32 *count, U32 *mediaType);
S32 ippiAP2DBSetBluetoothStatus(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IAP2_BLUETOOTH_INFO *btInfo);
S32 ippiAP2DBGetBluetoothStatus(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, U16 btID, U32 *profile);
S32 ippiAP2DBGetMediaType(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, U64 trackID, S32 *mediaType);
void ippiAP2PlaylistTrackCheck(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, IPOD_PLAYER_CORE_IAP2_DB_STATUS *dbStatus, IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST **list, BOOL *setflag);
BOOL ippiAP2DBCheckRevision(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, IPOD_PLAYER_CORE_IAP2_DB_STATUS *dbStatus);
void ippiAP2DBGetRevision(IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, U8 *memoryRevision);
S32 ippiAP2DBGetSelectingCategories(IPOD_PLAYER_IAP2_DB_HANDLE *dbHandle, IPOD_PLAYER_IAP2_DB_CATLIST **curList);
void ippiAP2DBFreeSelectingCategories(IPOD_PLAYER_IAP2_DB_CATLIST *curList);
void ippiAP2DBSetCapability(S32 mediaType, U32 *capability);

#endif /* IPP_IAP2_DATABASE_H */
