#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h> 
#include <libxml2/libxml/tree.h>
#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/encoding.h>
#include <libxml2/libxml/xmlwriter.h>
#include "pthread_adit.h"
#include "adit_typedef_linux.h"
#include "iap_commands.h"
#include "ipodcommon.h"
#include "iap_types.h"
#include "iPodPlayerCoreDef.h"
#include "iPodPlayerCoreiPodCtrlFunc.h"
#include "iap_common.h"
#include "iPodPlayerCoreCfg.h"
#include "iPodPlayerUtilityLog.h"


#define IPOD_PLAYER_GENIUS_NO_GET_VER 3

const IPOD_CORE_MASK_TBL ipodcore_feature_mask_tbl[] = 
{
/* Table list for general option */
    { IPODCORE_MASK_BIT_0,  IPOD_PLAYER_FEATURE_MASK_VIDEO_LINE_OUT       },  /* video line out          */
    { IPODCORE_MASK_BIT_1,  IPOD_PLAYER_FEATURE_MASK_VIDEO                },  /* video out put           */
    { IPODCORE_MASK_BIT_2,  IPOD_PLAYER_FEATURE_MASK_VIDEO_NTSC           },  /* ntsc signal format      */
    { IPODCORE_MASK_BIT_3,  IPOD_PLAYER_FEATURE_MASK_VIDEO_PAL            },  /* pal signal format       */
    { IPODCORE_MASK_BIT_4,  IPOD_PLAYER_FEATURE_MASK_VIDEO_COMPOSITE      },  /* composite connection    */
    { IPODCORE_MASK_BIT_5,  IPOD_PLAYER_FEATURE_MASK_VIDEO_SVIDEO         },  /* S-Video connection      */
    { IPODCORE_MASK_BIT_6,  IPOD_PLAYER_FEATURE_MASK_VIDEO_COMPONENT      },  /* component connection    */
    { IPODCORE_MASK_BIT_7,  IPOD_PLAYER_FEATURE_MASK_VIDEO_CLOSED_CAPTION },  /* video closed caption    */
    { IPODCORE_MASK_BIT_8,  IPOD_PLAYER_FEATURE_MASK_VIDEO_ASPECT_4_3     },  /* video aspect ratio 4:3  */
    { IPODCORE_MASK_BIT_9,  IPOD_PLAYER_FEATURE_MASK_VIDEO_ASPECT_16_9    },  /* video aspect ratio 16:9 */
    { IPODCORE_MASK_BIT_10, IPOD_PLAYER_FEATURE_MASK_VIDEO_SUBTITLE       },  /* video subtitle          */
    { IPODCORE_MASK_BIT_11, IPOD_PLAYER_FEATURE_MASK_VIDEO_ALT_AUDIO      },  /* alternate audio channel */
    { IPODCORE_MASK_BIT_13, IPOD_PLAYER_FEATURE_MASK_IOSAPP               },  /* isoApp communication    */
    { IPODCORE_MASK_BIT_14, IPOD_PLAYER_FEATURE_MASK_DEVICE_NOTIFICATION  },  /* device notification     */
    { IPODCORE_MASK_BIT_24, IPOD_PLAYER_FEATURE_MASK_APP_START            }   /* application launch      */
};

const IPOD_CORE_MASK_TBL ipodcore_event_mask_tbl[] = 
{
/* Table list for notification event option */
    { IPODCORE_MASK_BIT_3,  IPOD_PLAYER_EVENT_MASK_RADIO_TAGGING },  /* raido tagging          */
    { IPODCORE_MASK_BIT_4,  IPOD_PLAYER_EVENT_MASK_CAMERA        },  /* camear status          */
    { IPODCORE_MASK_BIT_5,  IPOD_PLAYER_EVENT_MASK_CHARGING      },  /* charging inof          */
    { IPODCORE_MASK_BIT_9,  IPOD_PLAYER_EVENT_MASK_DATABASE      },  /* database changed       */
    { IPODCORE_MASK_BIT_10, IPOD_PLAYER_EVENT_MASK_IOSAPP        },  /* running iosApp name    */
    { IPODCORE_MASK_BIT_15, IPOD_PLAYER_EVENT_MASK_OUT           },  /* iPodOut mode           */
    { IPODCORE_MASK_BIT_17, IPOD_PLAYER_EVENT_MASK_BT            },  /* bluetooth status       */
    { IPODCORE_MASK_BIT_19, IPOD_PLAYER_EVENT_MASK_IOSAPP_FULL   },  /* running iosApp status  */
    { IPODCORE_MASK_BIT_20, IPOD_PLAYER_EVENT_MASK_ASSISTIVE     }   /* assistive touch status */
};


const IPOD_CORE_MASK_TBL ipodcore_extoption_mask_tbl[] = 
{
/* Table list for notification event option */
    { IPODCORE_MASK_BIT_0, IPOD_PLAYER_FEATURE_MASK_VIDEO_BROWSING      },  /* video browsing     */
    { IPODCORE_MASK_BIT_1, IPOD_PLAYER_FEATURE_MASK_TRACK_INFO_RESTRICT },  /* track info         */
    { IPODCORE_MASK_BIT_3, IPOD_PLAYER_FEATURE_MASK_INTTELLIGENT_PL     },  /* nested playlist    */
    { IPODCORE_MASK_BIT_4, IPOD_PLAYER_FEATURE_MASK_DISPLAY_IMAGE       },  /* set display image  */
    { IPODCORE_MASK_BIT_7, IPOD_PLAYER_FEATURE_MASK_UID                 }   /* UID based commands */
};

const IPOD_CORE_MASK_TBL ipodcore_eventnotify_mask_tbl[] = 
{
/* Table list for notification event option */
    { IPOD_PLAYER_EVENT_MASK_RADIO_TAGGING, IPOD_PLAYER_EVENT_NOTIFICATION_MASK_TAGGING   },  /* tagging status          */
    { IPOD_PLAYER_EVENT_MASK_CAMERA,        IPOD_PLAYER_EVENT_NOTIFICATION_MASK_CAMERA    },  /* camear status           */
    { IPOD_PLAYER_EVENT_MASK_CHARGING,      IPOD_PLAYER_EVENT_NOTIFICATION_MASK_CHARGING  },  /* charging inof           */
    { IPOD_PLAYER_EVENT_MASK_IOSAPP,        IPOD_PLAYER_EVENT_NOTIFICATION_MASK_APPNAME   },  /* iosApp name             */
    { IPOD_PLAYER_EVENT_MASK_OUT,           IPOD_PLAYER_EVENT_NOTIFICATION_MASK_OUT       },  /* iPodOut status          */
    { IPOD_PLAYER_EVENT_MASK_BT,            IPOD_PLAYER_EVENT_NOTIFICATION_MASK_BT        },  /* bluetooth status        */
    { IPOD_PLAYER_EVENT_MASK_IOSAPP_FULL,   IPOD_PLAYER_EVENT_NOTIFICATION_MASK_APPSTATUS },  /* iosApp status           */
    { IPOD_PLAYER_EVENT_MASK_ASSISTIVE,     IPOD_PLAYER_EVENT_NOTIFICATION_MASK_ASSISTIBE }   /* assistive touch status  */
};

const IPOD_CORE_CATEGORY_MAPPING_TBL ipodcore_category_mapping_tbl[] = 
{
/* Table list for Category option */
    { IPOD_PLAYER_DB_TYPE_ALL,            IPOD_CAT_TOPLIST         },  /* category is Toplist   */
    { IPOD_PLAYER_DB_TYPE_PLAYLIST,       IPOD_CAT_PLAYLIST        },  /* category is Playlist  */
    { IPOD_PLAYER_DB_TYPE_ARTIST,         IPOD_CAT_ARTIST          },  /* category is Artist    */
    { IPOD_PLAYER_DB_TYPE_ALBUM,          IPOD_CAT_ALBUM           },  /* category is Album     */
    { IPOD_PLAYER_DB_TYPE_GENRE,          IPOD_CAT_GENRE           },  /* category is Genre     */
    { IPOD_PLAYER_DB_TYPE_TRACK,          IPOD_CAT_TRACK           },  /* category is Track     */
    { IPOD_PLAYER_DB_TYPE_COMPOSER,       IPOD_CAT_COMPOSER        },  /* category is Composer  */
    { IPOD_PLAYER_DB_TYPE_AUDIOBOOK,      IPOD_CAT_AUDIOBOOK       },  /* category is Audiobook by 1.06 or more */
    { IPOD_PLAYER_DB_TYPE_PODCAST,        IPOD_CAT_PODCAST         },  /* category is Podcast by 1.08 or more   */
    { IPOD_PLAYER_DB_TYPE_NESTED_PLAYLIST,IPOD_CAT_NESTED_PLAYLIST },  /* category is Nested playlist           */
    { IPOD_PLAYER_DB_TYPE_INTELLIGENT,    IPOD_CAT_GENIUS          },  /* category is Genius playlist           */
    { IPOD_PLAYER_DB_TYPE_ITUNESU,        IPOD_CAT_ITUNESU         }   /* category is iTunes University lecture */
};

const IPOD_CORE_RETCODE_MAPPING_TBL ipodcore_retcode_mapping_tbl[] = 
{
/* Table list for return code option */
    { IPOD_OK,                          IPOD_PLAYER_OK                              },  /* ret code is ok         */
    { IPOD_ERROR,                       IPOD_PLAYER_ERR_IPOD_CTRL_ERROR             },  /* ret code is ipod error */
    { IPOD_BAD_PARAMETER,               IPOD_PLAYER_ERR_INVALID_PARAMETER           },  /* ret code is bad param  */
    { IPOD_ERR_WRONG_MODE,              IPOD_PLAYER_ERR_INVALID_MODE                },
    { IPOD_ERR_ONLY_IN_ADVANCEDMODE,    IPOD_PLAYER_ERR_INVALID_MODE                },
    { IPOD_ALREADY_CONNECTED,           IPOD_PLAYER_ERR_DEVICE_ALREADY_CONNECTED    },
    { IPOD_NOT_CONNECTED,               IPOD_PLAYER_ERR_NOT_CONNECT                 },
    { IPOD_ERR_TMOUT,                   IPOD_PLAYER_ERR_IPOD_CTRL_TMOUT             },  /* ret code is timeout    */
    { IPOD_UNKNOWN_ID,                  IPOD_UNKNOWN_ID                             },  /* ret code is unknown id */
    { IPOD_ERR_NOMEM,                   IPOD_PLAYER_ERR_NOMEM                       }   /* ret code is no memory  */
};


S32 iPodCoreFuncChangeRetCodeToPlayer(S32 retCode)
{
    
    S32 rc = IPOD_PLAYER_ERR_IPOD_CTRL_ERROR;
    U32 ix = 0;
    U32 tblsize = 0;
    
    if(retCode == IPOD_PLAYER_ERR_INVALID_PARAMETER){
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    tblsize = sizeof(ipodcore_retcode_mapping_tbl)/sizeof(ipodcore_retcode_mapping_tbl[0]);
    
    /* table search */
    for(ix = 0; ix < tblsize; ix++){
        /* category is type of player */
        if(retCode == ipodcore_retcode_mapping_tbl[ix].ctrl_ret){
            /* change it to type of ipod  */
            rc = ipodcore_retcode_mapping_tbl[ix].player_ret;
            break;
        }
    }
    
    return rc;
}

static S32 iPodCoreFuncSleep(U32 timems)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    struct timespec waitTime;
    struct timespec remainTime;
    
    memset(&waitTime , 0, sizeof(waitTime));
    memset(&remainTime , 0, sizeof(remainTime));
    
    /* Change millisecond to second */
    waitTime.tv_sec     = timems / IPOD_PLAYER_TIME_MSEC_TO_SEC;
    /* Change millisend to nano second */
    waitTime.tv_nsec    = (timems % IPOD_PLAYER_TIME_MSEC_TO_SEC) * IPOD_PLAYER_TIME_MSEC_TO_NSEC;
    
    for(i = 0; i < IPODCORE_NANOSLEEP_RETRY_MAX; i++)
    {
        /* Sleep */
        rc = nanosleep(&waitTime, &remainTime);
        if(rc == 0)
        {
            rc = IPOD_PLAYER_OK;
            break;
        }
        else
        {
            /* Sleep was finised due to interrupt */
            if(errno == EINTR)
            {
                /* Set remaining time and retry nano sleep */
                waitTime.tv_sec = remainTime.tv_sec;
                waitTime.tv_nsec = remainTime.tv_nsec;
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, EINTR);
                rc = IPOD_PLAYER_ERROR;
            }
            else
            {
                rc = errno;
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
                rc = IPOD_PLAYER_ERROR;
                break;
            }
        }
    }
    
    return rc;
}

S32 iPodCoreFuncInitConnection(void)
{
    S32 rc = IPOD_ERROR;
    
    /* Initialize the iPod ctrl */
    rc = iPodInitConnection();
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

void iPodCoreFuncDisconnect(void)
{
    /* Deinitialize the iPod ctrl */
    iPodDisconnect();
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE);
    
    return;
}


S32 iPodCoreFuncSwitchAudio(U32 devID, U8 mode)
{
    S32 rc = IPOD_ERROR;
    IPOD_SWITCH_AUDIO_OUT audio = (IPOD_SWITCH_AUDIO_OUT)mode;
    
    
    /* Switch the audio output source to Analog, Digital */
    iPodSwitchAudioOutput(devID, audio);
    rc = IPOD_PLAYER_OK;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncStartAuthentication(U32 devID)
{
    S32 rc = IPOD_ERROR;
    
    /* Identify again */
    iPodRequestIdentify(devID);
    rc = IPOD_PLAYER_OK;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


S32 iPodCoreFuncPlay(U32 devID)
{
    S32 rc = IPOD_ERROR;
    
    /* Send the play toggle to iPod */
    rc = iPodPlayToggle(devID);
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncPause(U32 devID)
{
    S32 rc = IPOD_ERROR;
    
    /* Send the pause toggle to iPod */
    rc = iPodPlayToggle(devID);
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncStop(U32 devID)
{
    S32 rc = IPOD_ERROR;
    
    /* Send the stop to iPod*/
    rc = iPodPlayStop(devID);
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncNextTrack(U32 devID)
{
    S32 rc = IPOD_ERROR;
    
    /* Send the next track to iPod*/
    rc = iPodPlayNextTrack(devID);
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncPrevTrack(U32 devID)
{
    S32 rc = IPOD_ERROR;
    
    /* Send the previous track to iPod*/
    rc = iPodPlayPrevTrack(devID);
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncNextChapter(U32 devID)
{
    S32 rc = IPOD_ERROR;
    S32 chapterIndex = 0;
    S32 chapterCount = 0;
    
    /* Get the index of current playing chapter */
    rc = iPodGetCurrentPlayingTrackChapterInfo(devID, &chapterIndex, &chapterCount);
    if(rc == IPOD_OK)
    {
        /* Current playing track has chapter */
        if((chapterIndex >= 0) && (chapterCount >= 1))
        {
            /* Index of current playing chapter less than tail index */
            if(chapterIndex < (chapterCount - 1))
            {
                /* Send the next chapter to iPod */
                rc = iPodPlayNextChapter(devID);
                
                /* Change return code of iPod ctrl to player */
                rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
            }
            else
            {
                /* Current playing chapter is tail index. */
                rc = IPOD_PLAYER_ERR_INVALID_POSITION;
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, chapterIndex, chapterCount - 1);
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERR_NO_CHAPTER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, chapterIndex, chapterCount);
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_NO_CHAPTER;
        IPOD_DLT_WARN("iPodGetCurrentPlayingTrackChapterInfo :rc=%d", rc);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncPrevChapter(U32 devID)
{
    S32 rc = IPOD_ERROR;
    S32 chapterIndex = 0;
    S32 chapterCount = 0;
    
    
    /* Get the index of current playing chapter */
    rc = iPodGetCurrentPlayingTrackChapterInfo(devID, &chapterIndex, &chapterCount);
    if(rc == IPOD_OK)
    {
        /* Current playing track has chapter */
        if((chapterIndex >= 0) && (chapterCount >= 1))
        {
            /* Index of current playing chapter is not top index */
            if(chapterIndex != 0)
            {
                /* Send the previous chapter to iPod */
                rc = iPodPlayPrevChapter(devID);
                
                /* Change return code of iPod ctrl to player */
                rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
            }
            else
            {
                rc = IPOD_PLAYER_ERR_INVALID_POSITION;
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, chapterIndex);
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERR_NO_CHAPTER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, chapterIndex, chapterCount);
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_NO_CHAPTER;
        IPOD_DLT_WARN("iPodGetCurrentPlayingTrackChapterInfo :rc=%d", rc);
    }
    
    return rc;
}

S32 iPodCoreFuncFastForward(U32 devID)
{
    S32 rc = IPOD_ERROR;
    
    /* Send the fastforwad to iPod */
    rc = iPodPlayFastForward(devID);
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    if(rc != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncRewind(U32 devID)
{
    S32 rc = IPOD_ERROR;
    
    /* Send the rewind to iPod */
    rc = iPodPlayFastBackward(devID);
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncGotoTrackPosition(U32 devID, U32 timems)
{
    S32 rc = IPOD_ERROR;
    
    
    /* Send the set track position to iPod */
    rc = iPodSetTrackPosition(devID, timems);
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


S32 iPodCoreFuncPlayTrack(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackIndex)
{
    S32 rc = IPOD_ERROR;
    
    
    /* Check the type */
    switch(type)
    {
        /* Play by playback engine */
        case IPOD_PLAYER_TRACK_TYPE_PLAYBACK:
            /* Send the set current playing track to iPod */
            rc = iPodSetCurrentPlayingTrack(devID, (U32)trackIndex);
            break;
            
        /* Play by database engine */
        case IPOD_PLAYER_TRACK_TYPE_DATABASE:
            /* Send the play current selection to iPod */
            rc = iPodPlayCurrentSelection(devID, trackIndex);
            break;
            
        /* Play by UID */
        case IPOD_PLAYER_TRACK_TYPE_UID:
            rc = IPOD_BAD_PARAMETER;
            break;
        default:
            rc = IPOD_BAD_PARAMETER;
            break;
        
    }
    
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, type);
    }
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncSetRepeat(U32 devID, U8 status)
{
    S32 rc = IPOD_ERROR;
    
    
    /* Check the status */
    switch(status)
    {
        /* status is repeat off */
        case IPOD_PLAYER_REPEAT_OFF:
            /* Send the repeat off to iPod*/
            rc = iPodRepeatOff(devID);
            break;
            
        /* status is repeat one */
        case IPOD_PLAYER_REPEAT_ONE:
            /* Send the repeat current song to iPod */
            rc = iPodRepeatCurrentSong(devID);
            break;
            
        /* status is repeat all */
        case IPOD_PLAYER_REPEAT_ALL:
            /* Send the repeat all song to iPod */
            rc = iPodRepeatAllSongs(devID);
            break;

        default:
            rc = IPOD_BAD_PARAMETER;
            break;
    }
    
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, status);
    }
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncSetShuffle(U32 devID, U8 status)
{
    S32 rc = IPOD_ERROR;
    
    
    /* Check the shuffle status */
    switch(status)
    {
        /* Status is shuffle off */
        case IPOD_PLAYER_SHUFFLE_OFF:
            /* Send the shuffle off to iPod */
            rc = iPodShuffleOff(devID);
            break;
            
        /* Status is shuffle tracks */
        case IPOD_PLAYER_SHUFFLE_TRACKS:
            /* Send the shuffle songs to iPod */
            rc = iPodShuffleOnSongs(devID);
            break;
            
        /* Status is shuffle albums */
        case IPOD_PLAYER_SHUFFLE_ALBUMS:
            /* Send the shuffle albums to iPod */
            rc = iPodShuffleOnAlbums(devID);
            break;
            
        default:
            rc = IPOD_BAD_PARAMETER;
            break;
    }
    
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, status);
    }
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;

}

S32 iPodCoreFuncSetEqualizer(U32 devID, U32 eq, U8 restore)
{
    S32 rc = IPOD_ERROR;
    
    
    /* Send the set current equlizer index to iPod */
    rc = iPodSetCurrentEQProfileIndex(devID, eq, restore);
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;

}


S32 iPodCoreFuncSetMode(U32 devID, U8 mode)
{
    S32 rc = IPOD_ERROR;
    IPOD_EXTENDED_STATUS_CHANGE_NOTIFICATION notify;
    U32 remoteEventNotifyMask = 0;
    
    /* Initialize the structure */
    memset(&notify, 0, sizeof(notify));

    
    /* Check the mode */
    switch(mode)
    {
        /* Mode is simple mode */
        case IPOD_PLAYER_MODE_SELF_CONTROL:
            /* Send the enter simple mode to iPod */
            rc = iPodEnterSimpleMode(devID);
            break;
            
        /* Mode is extend mode*/
        case IPOD_PLAYER_MODE_REMOTE_CONTROL:
            /* activate shuffle and repeat remote event notification */
            remoteEventNotifyMask = (U32)0x180;
            rc = iPodSetRemoteEventNotification(devID, remoteEventNotifyMask);
            if(rc == IPOD_OK)
            {
                /* Send the enter extend mode to iPod */
                rc = iPodEnterExtendedInterfaceMode(devID);
                if(rc == IPOD_OK)
                {
                    /* Set the notification bit */
                    notify.basicPlay = 1;
                    notify.extendedPlay = 1;
                    notify.trackIndex = 1;
                    notify.trackTimeMs = 1;
                    notify.chapterIndex = 1;
                    notify.chapterTimeMs = 1;
                    notify.trackUID = 1;
                    notify.trackMediaType = 1;
                    notify.trackLyrics = 1;
                    notify.capChanges = 1;
                    
                    /* Send the set extend notification to iPod */
                    rc = iPodExtendedSetPlayStatusChangeNotification(devID, notify);
                    if(rc == IPOD_BAD_PARAMETER)
                    {
                        notify.capChanges = 0;
                        rc = iPodExtendedSetPlayStatusChangeNotification(devID, notify);
                        if(rc == IPOD_BAD_PARAMETER)
                        {
                            /* Old iPod does not support extend notification 
                             * Send the old notification to iPod */
                            rc = iPodSetPlayStatusChangeNotification(devID, IPOD_STATUS_CHANGE_NOTIFICATION_ENABLE);
                            if(rc == IPOD_OK)
                            {
                                /* Connected iPod is old */
                                rc = 1;
                            }
                            else
                            {
                                rc = IPOD_ERROR;
                            }
                        }
                    }
                    else if(rc < IPOD_OK)
                    {
                        rc = IPOD_ERROR;
                    }
                }
            }
            break;
        
        /* Mode is out mode */
        case IPOD_PLAYER_MODE_HMI_CONTROL:
            rc = iPodSetUIMode(devID, 2);
            break;
        default:
            rc = IPOD_BAD_PARAMETER;
            break;
    }
    
    if(rc != 1){
        /* Change return code of iPod ctrl to player */
        rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
        if(rc != IPOD_PLAYER_OK)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreSetBit(U64 option, const IPOD_CORE_MASK_TBL *mask_tbl, U32 tblsize, U64 *mask)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 ix = 0;
    
    /* Parameter check */
    if((mask == NULL) || (mask_tbl == NULL)){
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, mask, mask_tbl);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* table search */
    for(ix = 0; ix < tblsize; ix++){
        /* mask is set */
        if(is_setbit(option, mask_tbl[ix].opt_mask)){
            /* set bit */
            *mask |= mask_tbl[ix].flag;
        }
    }
    rc = IPOD_PLAYER_OK;
    
    return rc;
}

S32 iPodCoreFuncGetGeneralOption(U32 devID, U32 *mask)
{
    S32 rc = IPOD_ERROR;
    U64 option = 0;
    U32 tblsize =0;
    
    if(mask == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, mask);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the options of General Lingo */
    rc = iPodGetiPodOptionsForLingo(devID, IPOD_LINGO_GENERAL, &option);
    
    /* iPod supports OptionsForLingo. It means that iPod is new one */
    if(rc == IPOD_OK)
    {
        /* set iPod feature mask */
        tblsize = sizeof(ipodcore_feature_mask_tbl)/sizeof(ipodcore_feature_mask_tbl[0]);
        rc = iPodCoreSetBit(option, ipodcore_feature_mask_tbl, tblsize, (U64*)(void*)mask);
    }
    else
    {
        IPOD_DLT_WARN("iPodGetiPodOptionsForLingo :rc=%d", rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncGetExtendOption(U32 devID, U32 *mask)
{
    S32 rc = IPOD_ERROR;
    U64 option = 0;
    U32 tblsize =0;
    
    if(mask == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, mask);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the options of Extended lingo */
    rc = iPodGetiPodOptionsForLingo(devID, IPOD_LINGO_EXTENDED_INTERFACE, &option);
    if(rc == IPOD_OK)
    {
        
        /* set iPod feature mask */
        tblsize = sizeof(ipodcore_extoption_mask_tbl)/sizeof(ipodcore_extoption_mask_tbl[0]);
        rc = iPodCoreSetBit(option, ipodcore_extoption_mask_tbl, tblsize, (U64*)(void*)mask);
    }
    else
    {
        IPOD_DLT_WARN("iPodGetiPodOptionsForLingo :rc=%d", rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncGetDigitalOption(U32 devID, U32 *mask)
{
    S32 rc = IPOD_ERROR;
    U64 option = 0;
    
    if(mask == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, mask);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the options of Digital audio lingo */
    rc = iPodGetiPodOptionsForLingo(devID, IPOD_LINGO_DIGITAL_AUDIO, &option);
    if(rc == IPOD_OK)
    {
        /* iPod supports video delay */
        if((option & IPODCORE_MASK_BIT_0) == IPODCORE_MASK_BIT_0)
        {
            *mask |= IPOD_PLAYER_FEATURE_MASK_VIDEO_DELAY;
        }
    }
    else
    {
        IPOD_DLT_WARN("iPodGetiPodOptionsForLingo :rc=%d", rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}



S32 iPodCoreFuncGetStorageOption(U32 devID, U32 *mask)
{
    S32 rc = IPOD_ERROR;
    U64 option = 0;
    
    if(mask == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, mask);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the options of Storage lingo */
    rc = iPodGetiPodOptionsForLingo(devID, IPOD_LINGO_STORAGE, &option);
    if(rc == IPOD_OK)
    {
        /* iPod supports tagging */
        if((option & IPODCORE_MASK_BIT_0) == IPODCORE_MASK_BIT_0)
        {
            *mask |= IPOD_PLAYER_FEATURE_MASK_SONG_TAG;
        }
    }
    else
    {
        IPOD_DLT_WARN("iPodGetiPodOptionsForLingo :rc=%d", rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


S32 iPodCoreFuncGetiPodOutOption(U32 devID, U32 *mask)
{
    S32 rc = IPOD_ERROR;
    U64 option = 0;
    
    if(mask == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, mask);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the options of iPodOut lingo */
    rc = iPodGetiPodOptionsForLingo(devID, IPOD_LINGO_OUT, &option);
    if(rc == IPOD_OK)
    {
        /* iPod supports iPodOut */
        *mask |= IPOD_PLAYER_FEATURE_MASK_HMI_CONTROL;
    }
    else
    {
        IPOD_DLT_WARN("iPodGetiPodOptionsForLingo :rc=%d", rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncGetLocationOption(U32 devID, U32 *mask)
{
    S32 rc = IPOD_ERROR;
    U64 option = 0;
    
    if(mask == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, mask);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the options of Location lingo */
    rc = iPodGetiPodOptionsForLingo(devID, IPOD_LINGO_LOCATION, &option);
    if(rc == IPOD_OK)
    {
        /* iPod supports location lingo */
        *mask |= IPOD_PLAYER_FEATURE_MASK_GPS;
    }
    else
    {
        IPOD_DLT_WARN("iPodGetiPodOptionsForLingo :rc=%d", rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncGetGeneralVersion(U32 devID, U32 *mask)
{
    S32 rc = IPOD_ERROR;
    U8 iPodMajor = 0;
    U8 iPodMinor = 0;
    
    if(mask == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, mask);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the protocol version of general lingo */
    rc = iPodGetLingoProtocolVersion(devID, IPOD_LINGO_GENERAL, &iPodMajor, &iPodMinor);
    if(rc == IPOD_OK)
    {
        /* General lingo version is 1.05 or more */
        if((iPodMajor >= IPODCORE_VER_MAJOR_1)  && (iPodMinor >= IPODCORE_VER_MINOR_5))
        {
            /* It assumes to support all of video function */
            *mask |= IPOD_PLAYER_FEATURE_MASK_VIDEO;
            *mask |= IPOD_PLAYER_FEATURE_MASK_VIDEO_LINE_OUT;
            *mask |= IPOD_PLAYER_FEATURE_MASK_VIDEO_NTSC;
            *mask |= IPOD_PLAYER_FEATURE_MASK_VIDEO_PAL;
            *mask |= IPOD_PLAYER_FEATURE_MASK_VIDEO_COMPOSITE;
            *mask |= IPOD_PLAYER_FEATURE_MASK_VIDEO_SVIDEO;
            *mask |= IPOD_PLAYER_FEATURE_MASK_VIDEO_COMPONENT;
            *mask |= IPOD_PLAYER_FEATURE_MASK_VIDEO_CLOSED_CAPTION;
            *mask |= IPOD_PLAYER_FEATURE_MASK_VIDEO_ASPECT_4_3;
            *mask |= IPOD_PLAYER_FEATURE_MASK_VIDEO_ASPECT_16_9;
            *mask |= IPOD_PLAYER_FEATURE_MASK_VIDEO_ALT_AUDIO;
            *mask |= IPOD_PLAYER_FEATURE_MASK_VIDEO_SUBTITLE;
        }
        
        /* General lingo version is 1.09 or more */
        if((iPodMajor >= IPODCORE_VER_MAJOR_1) && (iPodMinor >= IPODCORE_VER_MINOR_9))
        {
            /* It assumes to support all of iOS communication function */
            *mask |= IPOD_PLAYER_FEATURE_MASK_IOSAPP;
            *mask |= IPOD_PLAYER_FEATURE_MASK_DEVICE_NOTIFICATION;
            *mask |= IPOD_PLAYER_FEATURE_MASK_APP_START;
        }
    }
    else
    {
        IPOD_DLT_WARN("iPodGetLingoProtocolVersion :rc=%d", rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


S32 iPodCoreFuncGetExtendVersion(U32 devID, U32 *mask)
{
    S32 rc = IPOD_ERROR;
    U8 iPodMajor = 0;
    U8 iPodMinor = 0;
    
    if(mask == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, mask);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the protocol version of Extended interface lingo */
    rc = iPodGetLingoProtocolVersion(devID, IPOD_LINGO_EXTENDED_INTERFACE, &iPodMajor, &iPodMinor);
    if(rc == IPOD_OK)
    {
        /* Extended lingo version is 1.11 or more */
        if((iPodMajor >= IPODCORE_VER_MAJOR_1) && (iPodMinor >= IPODCORE_VER_MINOR_11))
        {
            /* It assumes to support video browsing and set the display image */
            *mask |= IPOD_PLAYER_FEATURE_MASK_VIDEO_BROWSING;
            *mask |= IPOD_PLAYER_FEATURE_MASK_DISPLAY_IMAGE;
        }
        
        /* Extended lingo version is 1.13 ore more */
        if((iPodMajor >= IPODCORE_VER_MAJOR_1) && (iPodMinor >= IPODCORE_VER_MINOR_13))
        {
            /* It assumes to support genius playlist and UID, DB and PB trackInfo */
            *mask |= IPOD_PLAYER_FEATURE_MASK_INTTELLIGENT_PL;
            *mask |= IPOD_PLAYER_FEATURE_MASK_TRACK_INFO_RESTRICT;
        }
        
        /* Extended lingo version is 1.14 or more */
        if((iPodMajor >= IPODCORE_VER_MAJOR_1) && (iPodMinor >= IPODCORE_VER_MINOR_14))
        {
            /* It assumes to support UID based commands */
            *mask |= IPOD_PLAYER_FEATURE_MASK_UID;
        }
    }
    else
    {
        IPOD_DLT_WARN("iPodGetLingoProtocolVersion :rc=%d", rc);
    }

    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncGetDigitalVersion(U32 devID, U32 *mask)
{
    S32 rc = IPOD_ERROR;
    U8 iPodMajor = 0;
    U8 iPodMinor = 0;
    
    if(mask == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, mask);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the protocol version of Digital audio lingo */
    rc = iPodGetLingoProtocolVersion(devID, IPOD_LINGO_DIGITAL_AUDIO, &iPodMajor, &iPodMinor);
    if(rc == IPOD_OK)
    {
        /* Digital audio lingo version is 1.03 or more */
        if((iPodMajor >= IPODCORE_VER_MAJOR_1) && (iPodMinor >= IPODCORE_VER_MINOR_3))
        {
            /* It assumes to support video delay */
            *mask |= IPOD_PLAYER_FEATURE_MASK_VIDEO_DELAY;
        }
    }
    else
    {
        IPOD_DLT_WARN("iPodGetLingoProtocolVersion :rc=%d", rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncGetStorageVersion(U32 devID, U32 *mask)
{
    S32 rc = IPOD_ERROR;
    U8 iPodMajor = 0;
    U8 iPodMinor = 0;
    
    if(mask == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, mask);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the protocol version of Storage lingo */
    rc = iPodGetLingoProtocolVersion(devID, IPOD_LINGO_STORAGE, &iPodMajor, &iPodMinor);
    if(rc == IPOD_OK)
    {
        /* Storage lingo version is 1.01 or more */
        if((iPodMajor >= IPODCORE_VER_MAJOR_1) && (iPodMinor >= IPODCORE_VER_MINOR_2))
        {
            /* It assumes to support tagging */
            *mask |= IPOD_PLAYER_FEATURE_MASK_SONG_TAG;
        }
    }
    else
    {
        IPOD_DLT_WARN("iPodGetLingoProtocolVersion :rc=%d", rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncGetiPodOutVersion(U32 devID, U32 *mask)
{
    S32 rc = IPOD_ERROR;
    U8 iPodMajor = 0;
    U8 iPodMinor = 0;
    
    if(mask == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, mask);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the protocol version of iPodOut lingo */
    rc = iPodGetLingoProtocolVersion(devID, IPOD_LINGO_OUT, &iPodMajor, &iPodMinor);
    if(rc == IPOD_OK)
    {
        /* iPodOut lingo version is 1.00 or more */
        if(iPodMajor >= IPODCORE_VER_MAJOR_1)
        {
            /* It assumes to support iPodOut */
            *mask |= IPOD_PLAYER_FEATURE_MASK_HMI_CONTROL;
        }
    }
    else
    {
        IPOD_DLT_WARN("iPodGetLingoProtocolVersion :rc=%d", rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncGetLocationVersion(U32 devID, U32 *mask)
{
    S32 rc = IPOD_ERROR;
    U8 iPodMajor = 0;
    U8 iPodMinor = 0;
    
    if(mask == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, mask);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the protocol version of Location lingo */
    rc = iPodGetLingoProtocolVersion(devID, IPOD_LINGO_LOCATION, &iPodMajor, &iPodMinor);
    if(rc == IPOD_OK)
    {
        /* Location lingo version is 1.00 ore more */
        if(iPodMajor >= IPODCORE_VER_MAJOR_1)
        {
            /* It assuems to support location */
            *mask |= IPOD_PLAYER_FEATURE_MASK_GPS;
        }
    }
    else
    {
        IPOD_DLT_WARN("iPodGetLingoProtocolVersion :rc=%d", rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


S32 iPodCoreFuncGetPropertyOfFeature(U32 devID, U32 *mask)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    /* Parameter check */
    if(mask == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, mask);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the options of General Lingo */
    rc = iPodCoreFuncGetGeneralOption(devID, &*mask);
    /* iPod supports OptionsForLingo. It means that iPod is new one */
    if(rc == IPOD_PLAYER_OK)
    {
        iPodCoreFuncGetExtendOption(devID, &*mask);
        iPodCoreFuncGetDigitalOption(devID, &*mask);
        iPodCoreFuncGetStorageOption(devID, &*mask);
        iPodCoreFuncGetiPodOutOption(devID, &*mask);
        iPodCoreFuncGetLocationOption(devID, &*mask);
    }
    
    /* iPod does not support options for lingo. It means that iPod is old one.
     * Check the protocol version of each lingo */
    else
    {
        iPodCoreFuncGetGeneralVersion(devID, &*mask);
        iPodCoreFuncGetExtendVersion(devID, &*mask);
        iPodCoreFuncGetDigitalVersion(devID, &*mask);
        iPodCoreFuncGetStorageVersion(devID, &*mask);
        iPodCoreFuncGetiPodOutVersion(devID, &*mask);
        iPodCoreFuncGetLocationVersion(devID, &*mask);
    }
    
    rc = IPOD_PLAYER_OK;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncGetPropertyOfCurNotification(U32 devID, U64 *mask)
{
    S32 rc = IPOD_ERROR;
    U64 event = 0;
    U32 tblsize =0;
    /* Parameter check */
    if(mask == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, mask);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get current notification type */
    rc = iPodGetEventNotification(devID, &event);
    if(rc == IPOD_OK)
    {
        /* set iPod notification event mask */
        tblsize = sizeof(ipodcore_event_mask_tbl)/sizeof(ipodcore_event_mask_tbl[0]);
        rc = iPodCoreSetBit(event, ipodcore_event_mask_tbl, tblsize, mask);
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncGetPropertyOfSupportedNotification(U32 devID, U64 *mask)
{
    S32 rc = IPOD_ERROR;
    U64 event = 0;
    U32 tblsize =0;
    
    /* Parameter check */
    if(mask == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, mask);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the supported notification event */
    rc = iPodGetSupportedEventNotification(devID, &event);
    if(rc == IPOD_OK)
    {
        /* set iPod notification event mask */
        tblsize = sizeof(ipodcore_event_mask_tbl)/sizeof(ipodcore_event_mask_tbl[0]);
        rc = iPodCoreSetBit(event, ipodcore_event_mask_tbl, tblsize, mask);
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}
S32 iPodCoreFuncGetDeviceProperty(U32 devID, U32 mask, IPOD_PLAYER_DEVICE_PROPERTY *property)
{
    S32 rc = IPOD_ERROR;
    U8 rev = 0;
    IPOD_ARTWORK_FORMAT format[IPOD_PLAYER_FORMAT_ARRAY_MAX];
    IPOD_DISPLAY_IMAGE_LIMITS imageLimit[IPOD_PLAYER_FORMAT_ARRAY_MAX];
    U16 count = 0;
    U8 pixelFormat = 0;
    U16 i = 0;
    U8 *temp = NULL;
    
    /* Parameter check */
    if(property == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, property);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    /* Initialize the structure */
    memset(format, 0, sizeof(format));
    memset(imageLimit, 0, sizeof(imageLimit));
    
    /* Device name was set in mask */
    if(is_setbit(mask, IPOD_PLAYER_DEVICE_MASK_NAME))
    {
        temp = calloc(iPodGetMaxPayloadSize(devID), sizeof(U8));
        if(temp != NULL)
        {
            /* Get the name of iPod */
            iPodGetIPodName(devID, temp);
            strncpy((char *)property->name, (const char*)temp, IPOD_PLAYER_DEVICE_NAME_LEN_MAX);
            property->name[IPOD_PLAYER_DEVICE_NAME_LEN_MAX - 1] = '\0';
            free(temp);
            temp = NULL;
        }
    }
    
    /* Software version was set in mask */
    if(is_setbit(mask, IPOD_PLAYER_DEVICE_MASK_SOFT_VERSION))
    {
        /* Get the software version of iPod */
        iPodGetSoftwareVersion(devID, &property->softVer.majorVer, &property->softVer.minorVer, &rev);
    }
    
    /* Serial number was set in mask */
    if(is_setbit(mask, IPOD_PLAYER_DEVICE_MASK_SERIAL_NUMBER))
    {
        temp = calloc(iPodGetMaxPayloadSize(devID), sizeof(U8));
        if(temp != NULL)
        {
            /* Get the serial number of iPod */
            iPodGetSerialNumber(devID, temp);
            strncpy((char *)property->serial, (const char*)temp, IPOD_PLAYER_SERIAL_LEN_MAX);
            free(temp);
            temp = NULL;
        }
    }
    
    /* Max payload size was set in mask */
    if(is_setbit(mask, IPOD_PLAYER_DEVICE_MASK_MAX_PAYLOAD_SIZE))
    {
        /* Get the maximum payload size of iPod */
        rc = iPodRequestTransportMaxPacketSize(devID, &property->maxPayload);
        if(rc != IPOD_OK)
        {
            property->maxPayload = IPODCORE_DEFAULT_PAYLOAD_SIZE;
        }
        else
        {
            property->maxPayload = property->maxPayload - IPODCORE_START_HEADER_MAX;
        }
    }
    
    /* SupportedFeatureMask was set in mask */
    if(is_setbit(mask, IPOD_PLAYER_DEVICE_MASK_SUPPORTED_FEATURE))
    {
        iPodCoreFuncGetPropertyOfFeature(devID, &property->supportedFeatureMask);
    }
    
    /* Device event was set in mask */
    if(is_setbit(mask, IPOD_PLAYER_DEVICE_MASK_EVENT))
    {
        /* iPod supports event notification */
        if((property->supportedFeatureMask & IPOD_PLAYER_FEATURE_MASK_DEVICE_NOTIFICATION) == IPOD_PLAYER_FEATURE_MASK_DEVICE_NOTIFICATION)
        {
            /* Get the current notification event */
            iPodCoreFuncGetPropertyOfCurNotification(devID, &property->curEvent);
            
            /* Get the supported notification event */
            iPodCoreFuncGetPropertyOfSupportedNotification(devID, &property->supportEvent);
        }
    }
    
    /* File space for tagging was set in mask */
    if(is_setbit(mask, IPOD_PLAYER_DEVICE_MASK_FILE_SPACE))
    {
        /* iPod supports tagging feature */
        if((property->supportedFeatureMask & IPOD_PLAYER_FEATURE_MASK_SONG_TAG) == IPOD_PLAYER_FEATURE_MASK_SONG_TAG)
        {
            iPodGetiPodFreeSpace(devID, &property->fileSpace);
        }
    }
    
    /* Coverart format info was set in mask */
    if(is_setbit(mask, IPOD_PLAYER_DEVICE_MASK_FORMAT))
    {
        U8 j = 0;
        iPodGetArtworkFormats(devID, format, &count);
        
        for(i = 0; i < count; i++)
        {
            if(format[i].pixelFormat == IPOD_PLAYER_PIXEL_FORMAT_LE)
            {
                property->format[j].formatId = format[i].formatID;
                property->format[j].imageWidth = format[i].imageWidth;
                property->format[j].imageHeight = format[i].imageHeight;
                j++;
            }
            
        }
        
        property->formatCount = (U32)j;
    }
    
    /* Monochrome image limit was set in mask */
    if(is_setbit(mask, IPOD_PLAYER_DEVICE_MASK_MONO_LIMIT))
    {
        iPodGetMonoDisplayImageLimits(devID, &property->mono.maxWidth, &property->mono.maxHeight, &pixelFormat);
    }
    
    /* Color image limit was set in mask */
    if(is_setbit(mask, IPOD_PLAYER_DEVICE_MASK_COLOR_LIMIT))
    {
        iPodGetColorDisplayImageLimits(devID, imageLimit, &count);
        property->color.maxWidth = imageLimit[0].width;
        property->color.maxHeight = imageLimit[0].height;
    }

    rc = IPOD_PLAYER_OK;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


S32 iPodCoreFuncSetVideoDelay(U32 devID, U32 delayTime)
{
    S32 rc = IPOD_ERROR;
    
    
    /* Delay the video */
    rc = iPodSetVideoDelay(devID, delayTime);
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
    
}

S32 iPodCoreFuncGetNumEQ(U32 devID, U32 *maxEQ)
{
    S32 rc = IPOD_ERROR;
    
    if(maxEQ == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, maxEQ);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    rc = iPodGetNumEQProfiles(devID, maxEQ);
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncSetVideoSetting(U32 devID, const IPOD_PLAYER_VIDEO_SETTING *setting, U8 restore)
{
    S32 rc = IPOD_ERROR;
    IPOD_PREFERENCE_CLASS_ID classId = IPOD_VOUT_SETTING;
    IPOD_PREFERENCE_SETTING_ID settingId = {IPOD_VOUT_SETTING_OFF};
    U32 count = 0;
    U32 mask = 0;
    S32 rctemp = 0;
    
    
    for(count = 0; count < IPODCORE_BIT_MAX_32; count++)
    {
        /* set mask shits until count */
        mask = setting->videoSettingMask & (1 << count);
        
        /* mask is video output */
        if(mask == IPOD_PLAYER_VIDEO_OUTPUT)
        {
            classId = IPOD_VOUT_SETTING;
            settingId.videoOutSetting = (IPOD_VOUT_SETTING_SETTING_ID)setting->videoOutput;
        }
        
        /* mask is screen configuration */
        else if(mask == IPOD_PLAYER_VIDEO_SCREEN)
        {
            classId = IPOD_VSCREEN_CFG;
            settingId.screenCfg = (IPOD_VSCREEN_CFG_SETTING_ID)setting->screenSetting;
        }
        
        /* mask is video signal */
        else if(mask == IPOD_PLAYER_VIDEO_SIGNAL)
        {
            classId = IPOD_VSIG_FORMAT;
            settingId.signalFormat = (IPOD_VSIG_FORMAT_SETTING_ID)setting->videoSignal;
        }
        
        /* mask is video line out */
        else if(mask == IPOD_PLAYER_VIDEO_LINEOUT)
        {
            classId = IPOD_VLINE_OUT_USAGE;
            settingId.lineOut = (IPOD_VLINE_OUT_USAGE_SETTING_ID)setting->videoLineout;
        }
        
        /* mask is video calble */
        else if(mask == IPOD_PLAYER_VIDEO_CABLE)
        {
            classId = IPOD_VOUT_CONNECT;
            settingId.videoOutConnection = (IPOD_VOUT_CONNECT_SETTING_ID)setting->videoCable;
        }
        
        /* mask is video caption */
        else if(mask == IPOD_PLAYER_VIDEO_CAPTION)
        {
            classId = IPOD_VCLOSED_CAP;
            settingId.closedCaptioning = (IPOD_VCLOSED_CAP_SETTING_ID)setting->videoCaption;
        }
        
        /* mask is aspect ratio */
        else if(mask == IPOD_PLAYER_VIDEO_ASPECT)
        {
            classId = IPOD_VASP_RATIO;
            settingId.aspectRatio = (IPOD_VASP_RATIO_SETTING_ID)setting->aspectRatio;
        }
        
        /* mask is video subtitle */
        else if(mask == IPOD_PLAYER_VIDEO_SUBTITLE)
        {
            classId = IPOD_VSUBTITLES;
            settingId.subTitles = (IPOD_VSUBTITLES_SETTING_ID)setting->subtitle;
        }
        
        /* mask is alternate audio channel */
        else if(mask == IPOD_PLAYER_VIDEO_ALTAUDIO)
        {
            classId = IPOD_VALT_AUD_CHANNEL;
            settingId.audioChannel = (IPOD_VALT_AUD_CHANNEL_SETTING_ID)setting->altanateAudio;
        }
/* todo iPod ctrl does not support. 
        else if(mask == IPOD_PLAYER_VIDEO_POWER)
        {
            classId = IPOD_VPOWER;
            settingId.videoOutSetting = (IPOD_VPOWER_SETTING_ID)setting->power;
        }
        else if(mask == IPOD_PLAYER_VIDEO_VOICEOVER)
        {
            classId = IPOD_VVOICE_OVER;
            settingId.videoOutSetting = (IPOD_VVOICE_OVER_SETTING_ID)setting->voiceOver;
        }
*/
        else
        {
            rc = IPOD_BAD_PARAMETER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
        
        if(rc != IPOD_BAD_PARAMETER)
        {
            rc = iPodSetiPodPreferences(devID, classId, settingId, restore);
            if(rc != IPOD_OK)
            {
                rctemp = rc;
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
            }
        }
        
        rc = IPOD_OK;
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rctemp);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;

}

S32 iPodCoreFuncSetDisplayImage(U32 devID, const U32 imageSize, const U8 *image)
{
    S32 rc = IPOD_ERROR;
    
    
    /* Set display image */
    rc = iPodSetDisplayImageMemory(devID, image);
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc, imageSize);
    
    return rc;
}

S32 iPodCoreFuncSetPlaySpeed(U32 devID, IPOD_PLAYER_PLAYING_SPEED speed)
{
    S32 rc = IPOD_ERROR;
    IPOD_AUDIOBOOK_SPEED iPodSpeed = IPOD_AUDIOBOOK_SPEED_NORMAL;
    
    
    /* Check the request speed */
    switch(speed)
    {
        /* Request speed is slow */
        case IPOD_PLAYER_PLAYING_SPEED_SLOW:
            iPodSpeed = IPOD_AUDIOBOOK_SPEED_SLOW;
            rc = IPOD_OK;
            break;
            
        /* Request speed is normal */
        case IPOD_PLAYER_PLAYING_SPEED_NORMAL:
            iPodSpeed = IPOD_AUDIOBOOK_SPEED_NORMAL;
            rc = IPOD_OK;
            break;
            
        /* Request speed is fast */
        case IPOD_PLAYER_PLAYING_SPEED_FAST:
            iPodSpeed = IPOD_AUDIOBOOK_SPEED_FAST;
            rc = IPOD_OK;
            break;
            
        /* Request speed is undefined value */
        default:
            rc = IPOD_BAD_PARAMETER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, speed);
            break;
    }
    
    if(rc == IPOD_OK)
    {
        rc = iPodSetAudioBookSpeed(devID, iPodSpeed);
        if(rc != IPOD_OK)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
    }
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
    
}


S32 iPodCoreFuncSetDeviceEventNotification(U32 devID, U32 bitmask)
{
    S32 rc = IPOD_ERROR;
    U64 iPodMask = IPOD_PLAYER_EVENT_NOTIFICATION_MASK_FLOWCONTROL | IPOD_PLAYER_EVENT_NOTIFICATION_MASK_DATABASE;
    U32 tblsize = 0;
    
    
    /* set iPod notification mask */
    tblsize = sizeof(ipodcore_eventnotify_mask_tbl)/sizeof(ipodcore_eventnotify_mask_tbl[0]);
    rc = iPodCoreSetBit((U64)bitmask, ipodcore_eventnotify_mask_tbl, tblsize, &iPodMask);
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodSetEventNotification(devID, iPodMask);
        if(rc != IPOD_OK)
        {
            IPOD_DLT_WARN("iPodSetEventNotification :rc=%d", rc);
        }
        /* Change return code of iPod ctrl to player */
        rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncGetVideoSetting(U32 devID, IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_VIDEO_SETTING *setting, U32 mask)
{
    S32 rc = IPOD_ERROR;
    
    /* For lint */
    devID = devID;
    
    /* Parameter check */
    if((iPodCtrlCfg == NULL) || (setting == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, setting);
        rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    else
    {
        rc = IPOD_OK;
        setting->videoSettingMask = mask;
        
        /* Request to get the video output */
        if(is_setbit(mask, IPOD_PLAYER_VIDEO_OUTPUT))
        {
            setting->videoOutput = iPodCtrlCfg->iPodInfo->videoSetting.videoOutput;
        }
        
        /* Request to get the video screen configuration */
        if(is_setbit(mask, IPOD_PLAYER_VIDEO_SCREEN))
        {
            setting->screenSetting = iPodCtrlCfg->iPodInfo->videoSetting.screenSetting;
        }
        
        /* Request to get the video signal */
        if(is_setbit(mask, IPOD_PLAYER_VIDEO_SIGNAL))
        {
            setting->videoSignal = iPodCtrlCfg->iPodInfo->videoSetting.videoSignal;
        }
        
        /* Request to get the video line out*/
        if(is_setbit(mask, IPOD_PLAYER_VIDEO_LINEOUT))
        {
            setting->videoLineout = iPodCtrlCfg->iPodInfo->videoSetting.videoLineout;
        }
        
        /* Request to get the video cable setting */
        if(is_setbit(mask, IPOD_PLAYER_VIDEO_CABLE))
        {
            setting->videoCable = iPodCtrlCfg->iPodInfo->videoSetting.videoCable;
        }
        
        /* Request to get the closed captioning */
        if(is_setbit(mask, IPOD_PLAYER_VIDEO_CAPTION))
        {
            setting->videoCaption = iPodCtrlCfg->iPodInfo->videoSetting.videoCaption;
        }
        
        /* Request to get the aspect ratio */
        if(is_setbit(mask, IPOD_PLAYER_VIDEO_ASPECT))
        {
            setting->aspectRatio = iPodCtrlCfg->iPodInfo->videoSetting.aspectRatio;
        }
        
        /* Request to get the subtitle */
        if(is_setbit(mask, IPOD_PLAYER_VIDEO_SUBTITLE))
        {
            setting->subtitle = iPodCtrlCfg->iPodInfo->videoSetting.subtitle;
        }
        
        /* Request to get the alternate audio channel */
        if(is_setbit(mask, IPOD_PLAYER_VIDEO_ALTAUDIO))
        {
            setting->altanateAudio = iPodCtrlCfg->iPodInfo->videoSetting.altanateAudio;
        }
        
        /* Request to get the power status */
        if(is_setbit(mask, IPOD_PLAYER_VIDEO_POWER))
        {
            setting->power = iPodCtrlCfg->iPodInfo->videoSetting.power;
        }
        
        /* Request to get the voice over */
        if(is_setbit(mask, IPOD_PLAYER_VIDEO_VOICEOVER))
        {
            setting->voiceOver = iPodCtrlCfg->iPodInfo->videoSetting.voiceOver;
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncGetCoverartInfo(U32 devID, const IPOD_PLAYER_TRACK_TYPE type, const U64 trackIndex, const U16 formatId, U32 *timeCount, U32 *coverartTime, U32 size)
{
    S32 rc = IPOD_ERROR;
    
    /* For lint. In the future, ipod ctrl will be changed. then devID is used */
    size = size;
    
    /* Parameter check */
    if((timeCount == NULL) || (coverartTime == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, timeCount, coverartTime);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    
    /* Check the type of coverart info */
    switch(type)
    {
        /* Get the coverart info from track playback */
        case IPOD_PLAYER_TRACK_TYPE_PLAYBACK:
            rc = iPodGetTrackArtworkTimes(devID, trackIndex, formatId, 0, (U16)-1, (U16 *)timeCount, coverartTime);
            break;
            
        /* Get the coverart info from track database */
        case IPOD_PLAYER_TRACK_TYPE_DATABASE:
            rc = iPodGetTypeOfTrackArtworkTimes(devID, TYPE_DB, trackIndex, formatId, 0, (U16)-1, (U16 *)timeCount, coverartTime);
            break;
            
        /* Get the coverart info from track UID */
        case IPOD_PLAYER_TRACK_TYPE_UID:
            rc = iPodGetTypeOfTrackArtworkTimes(devID, TYPE_UID, trackIndex, formatId, 0, (U16)-1, (U16 *)timeCount, coverartTime);
            break;
            
        /* Get the coverart info from undefine value */
        default:
            rc = IPOD_BAD_PARAMETER;
            break;
    }
    
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, type);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncGetCoverart(U32 devID, const IPOD_PLAYER_TRACK_TYPE type, const U64 trackIndex, const U16 formatId, const U32 coverartTime, const IPOD_CB_GET_ARTWORK callback)
{
    S32 rc = IPOD_ERROR;
    
    /* Parameter check */
    if(callback == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, callback);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    
    /* Check the type of coverart */
    switch(type)
    {
        /* Get the coverart from track of playback */
        case IPOD_PLAYER_TRACK_TYPE_PLAYBACK:
            rc = iPodGetTrackArtworkData(devID, trackIndex, formatId, coverartTime, callback);
            break;
            
        /* Get the coverart from track of database */
        case IPOD_PLAYER_TRACK_TYPE_DATABASE:
            rc = iPodGetTypeOfTrackArtworkData(devID, TYPE_DB, trackIndex, formatId, coverartTime, callback);
            break;
            
        /* Get the coverart from track of UID */
        case IPOD_PLAYER_TRACK_TYPE_UID:
            rc = iPodGetTypeOfTrackArtworkData(devID, TYPE_UID, trackIndex, formatId, coverartTime, callback);
            break;
            
        /* Get the coverart from track of undefined value */
        default:
            rc = IPOD_BAD_PARAMETER;
            break;
    }
    
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, type);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncGetTrackInfoByPB(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U32 trackIndex, U32 *mask, U32 *curMask, U32 featureMask, IPOD_PLAYER_TRACK_INFO *info, const IPOD_CB_PLAYING_TRACK_INFO callback)
{
    S32 rc = IPOD_ERROR;
    U8 *temp = NULL;
    
    /* For lint. In the future, ipod ctrl will be changed. then devID is used */
    featureMask = featureMask;
    type = type;
    
    /* Parameter check */
    if((mask == NULL) || (curMask == NULL) || (info == NULL) || (callback == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, mask, curMask, info, callback);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Clear current setting mask */
    *curMask = 0;
    
    /* Request to get the track name */
    if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_TRACK_NAME))
    {
        temp = calloc(iPodGetMaxPayloadSize(devID), sizeof(U8));
        if(temp != NULL)
        {
            rc = iPodGetIndexedPlayingTrackTitle(devID, trackIndex, temp);
            if(rc == IPOD_OK)
            {
                strncpy((char *)info->trackName, (const char*)temp, IPOD_PLAYER_TRACK_NAME_LEN_MAX);
                info->trackName[IPOD_PLAYER_TRACK_NAME_LEN_MAX - 1] = '\0';
                /* Set the track name mask if track name could get */
                *curMask |= IPOD_PLAYER_TRACK_INFO_MASK_TRACK_NAME;
            }
            else
            {
                rc = IPOD_OK;
                *mask = *mask & (~IPOD_PLAYER_TRACK_INFO_MASK_TRACK_NAME);
            }
            free(temp);
            temp = NULL;
        }
        else
        {
            *mask = *mask & (~IPOD_PLAYER_TRACK_INFO_MASK_TRACK_NAME);
            rc = IPOD_OK;
        }
    }
    
    /* Request to get the album name */
    else if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_ALBUM_NAME))
    {
        temp = calloc(iPodGetMaxPayloadSize(devID), sizeof(U8));
        if(temp != NULL)
        {
            rc = iPodGetIndexedPlayingTrackAlbumName(devID, trackIndex, temp);
            if(rc == IPOD_OK)
            {
                strncpy((char *)info->albumName, (const char *)temp, IPOD_PLAYER_ALBUM_NAME_LEN_MAX);
                info->albumName[IPOD_PLAYER_ALBUM_NAME_LEN_MAX - 1] = '\0';
                /* Set the track name mask if album name could get */
                *curMask |= IPOD_PLAYER_TRACK_INFO_MASK_ALBUM_NAME;
            }
            else
            {
                *mask = *mask & (~IPOD_PLAYER_TRACK_INFO_MASK_ALBUM_NAME);
                rc = IPOD_OK;
            }
            free(temp);
            temp = NULL;
        }
        else
        {
            *mask = *mask & (~IPOD_PLAYER_TRACK_INFO_MASK_ALBUM_NAME);
            rc = IPOD_OK;
        }
    }

    /* Request to get the artist name */
    else if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_ARTIST_NAME))
    {
        temp = calloc(iPodGetMaxPayloadSize(devID), sizeof(U8));
        if(temp != NULL)
        {
            rc = iPodGetIndexedPlayingTrackArtistName(devID, trackIndex, temp);
            if(rc == IPOD_OK)
            {
                strncpy((char *)info->artistName, (const char*)temp, IPOD_PLAYER_ARTIST_NAME_LEN_MAX);
                info->artistName[IPOD_PLAYER_ARTIST_NAME_LEN_MAX - 1] = '\0';
                /* Set the track name mask if artist name could get */
                *curMask |= IPOD_PLAYER_TRACK_INFO_MASK_ARTIST_NAME;
            }
            else
            {
                *mask = *mask & (~IPOD_PLAYER_TRACK_INFO_MASK_ARTIST_NAME);
            }
            free(temp);
            temp = NULL;
        }
        else
        {
            *mask = *mask & (~IPOD_PLAYER_TRACK_INFO_MASK_ARTIST_NAME);
            rc = IPOD_OK;
        }
    }
    
    /* Request to get the podcast name */
    else if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_PODCAST_NAME))
    {
        rc = iPodGetIndexedPlayingTrackInfo(devID, IPOD_TRACK_PODCAST_NAME, trackIndex, 0, callback);
        if(rc != IPOD_OK)
        {
            rc = IPOD_OK;
            *mask = *mask & (~IPOD_PLAYER_TRACK_INFO_MASK_PODCAST_NAME);
        }
    }
    
    /* Request to get the description */
    else if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_DESCRIPTION))
    {
        rc = iPodGetIndexedPlayingTrackInfo(devID, IPOD_TRACK_DESCRIPTION, trackIndex, 0, callback);
        if(rc != IPOD_OK)
        {
            rc = IPOD_OK;
            *mask = *mask & (~IPOD_PLAYER_TRACK_INFO_MASK_DESCRIPTION);
        }
    }
    
    /* Request to get the lyric */
    else if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_LYRIC))
    {
        rc = iPodGetIndexedPlayingTrackInfo(devID, IPOD_TRACK_SONG_LYRICS, trackIndex, 0, callback);
        if(rc != IPOD_OK)
        {
            rc = IPOD_OK;
            *mask = *mask & (~IPOD_PLAYER_TRACK_INFO_MASK_LYRIC);
        }
    }
    
    /* Request to get the genre */
    else if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_GENRE))
    {
        rc = iPodGetIndexedPlayingTrackInfo(devID, IPOD_TRACK_GENRE, trackIndex, 0, callback);
        if(rc != IPOD_OK)
        {
            rc = IPOD_OK;
            *mask = *mask & (~IPOD_PLAYER_TRACK_INFO_MASK_GENRE);
        }
    }
    
    /* Request to get the composer */
    else if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_COMPOSER))
    {
        rc = iPodGetIndexedPlayingTrackInfo(devID, IPOD_TRACK_COMPOSER, trackIndex, 0, callback);
        if(rc != IPOD_OK)
        {
            rc = IPOD_OK;
            *mask = *mask & (~IPOD_PLAYER_TRACK_INFO_MASK_COMPOSER);
        }
    }
    
    /* Request to get the release date */
    else if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_RELEASE_DATE))
    {
        rc = iPodGetIndexedPlayingTrackInfo(devID, IPOD_TRACK_RELEASE_DATE, trackIndex, 0, callback);
        if(rc != IPOD_OK)
        {
            rc = IPOD_OK;
            *mask = *mask & (~IPOD_PLAYER_TRACK_INFO_MASK_RELEASE_DATE);
        }
    }
    
    /* Request to get the capability, track length or chapter count */
    else if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY) ||
        is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_TRACKLENGTH) ||
        is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_CHAPTER_COUNT))
    {
        rc = iPodGetIndexedPlayingTrackInfo(devID, IPOD_TRACK_CAP_INFO, trackIndex, 0, callback);
        if(rc != IPOD_OK)
        {
            rc = IPOD_OK;
            *mask = *mask & (~(IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY | IPOD_PLAYER_TRACK_INFO_MASK_TRACKLENGTH | IPOD_PLAYER_TRACK_INFO_MASK_CHAPTER_COUNT));
        }
    }
    
    /* Request to get the UID */
    else if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_UID))
    {
        //rc = iPodGetIndexedPlayingTrackInfo(IPOD_TRACK_PODCAST_NAME, trackIndex, contents->getTrackInfo->trackName);
        *mask = *mask & (~IPOD_PLAYER_TRACK_INFO_MASK_UID);
        rc = IPOD_OK;
    }
    
    /* Request to get the track kind */
    else if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_TRACK_KIND))
    {
        //rc = iPodGetIndexedPlayingTrackInfo(IPOD_TRACK_PODCAST_NAME, trackIndex, contents->getTrackInfo->trackName);
        *mask = *mask & (~IPOD_PLAYER_TRACK_INFO_MASK_TRACK_KIND);
        rc = IPOD_OK;
    }
    
    if(*mask == 0)
    {
        rc = IPOD_ERROR;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
        
}

S32 iPodCoreFuncGetTrackInfoType(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackIndex, S32 count, IPOD_TRACK_INFORMATION_BITFIELD trackType, IPOD_TRACK_INFORMATION_CB callback)
{
    S32 rc = IPOD_ERROR;
    
    /* Parameter check */
    if(callback == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, callback);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* For lint */
    count = count;
    
    /* Select TrackType*/
    switch(type)
    {
        /* Get the track info by playback */
        case IPOD_PLAYER_TRACK_TYPE_PLAYBACK:
            rc = iPodGetBulkPBTrackInfo(devID, (U32)trackIndex, 1, trackType, callback);
            break;
            
        /* Get the track info by database */
        case IPOD_PLAYER_TRACK_TYPE_DATABASE:
            rc = iPodGetBulkDBTrackInfo(devID, (U32)trackIndex, 1, trackType, callback);
            break;
            
        /* Get the track info by UID */
        case IPOD_PLAYER_TRACK_TYPE_UID:
            rc = iPodGetBulkUIDTrackInfo(devID, trackIndex, trackType, callback);
            break;
            
        /* Get the track info by undefined value */
        default:
            rc = IPOD_BAD_PARAMETER;
            break;
    }
    
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, type);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}
    
S32 iPodCoreFuncGetTrackInfoByType(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackIndex, U32 *mask, U32 *curMask, U32 featureMask, IPOD_PLAYER_TRACK_INFO *info, const IPOD_TRACK_INFORMATION_CB callback)
{
    S32 rc = IPOD_ERROR;
    IPOD_TRACK_INFORMATION_BITFIELD bitfield;
    
    /* For lint. In the future, ipod ctrl will be changed. then devID is used */
    featureMask = featureMask;
    
    /* Parameter check */
    if((mask == NULL) || (curMask == NULL) || (info == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, mask, curMask, info);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize parameter */
    memset(&bitfield, 0, sizeof(bitfield));
    
    
    /* Clear the current mask */
    *curMask = 0;
    rc = IPOD_OK;
    
    /* Request to get the track name */
    if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_TRACK_NAME))
    {
        bitfield.track_info.TRACK_NAME = 1;
    }

    /* Request to get the album name */
    if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_ALBUM_NAME))
    {
        bitfield.track_info.ALBUM_NAME = 1;
    }

    /* Request to get the artist name*/
    if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_ARTIST_NAME))
    {
        bitfield.track_info.ARTIST_NAME = 1;
    }
    
    if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_PODCAST_NAME))
    {
        /* Request to get the podcast name */
        if(type == IPOD_PLAYER_TRACK_TYPE_PLAYBACK)
        {
            if(callback == (void *)iPodCoreCBNotifyGetTrackInfo)
            {
                rc = iPodGetIndexedPlayingTrackInfo(devID, IPOD_TRACK_PODCAST_NAME, trackIndex, 0, iPodCoreCBNotifyOldGetTrackInfo);
            }
            else
            {
                rc = iPodGetIndexedPlayingTrackInfo(devID, IPOD_TRACK_PODCAST_NAME, trackIndex, 0, iPodCoreCBNotifyOldGetCurrentTrackInfo);
            }
            
            if((rc != IPOD_OK) && (rc != IPOD_UNKNOWN_ID))
            {
                rc = IPOD_OK;
                *mask = *mask & (~IPOD_PLAYER_TRACK_INFO_MASK_PODCAST_NAME);
            }
        }
        else
        {
            *mask = *mask & (~IPOD_PLAYER_TRACK_INFO_MASK_PODCAST_NAME);
        }
    }
    
    /* Request to get the description */
    if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_DESCRIPTION))
    {
        bitfield.track_info.DESCRIPTION = 1;
    }

    /* Request to get the lyric */
    if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_LYRIC))
    {
        bitfield.track_info.LYRIC_STRING = 1;
    }
    
    /* Request to get the genre */
    if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_GENRE))
    {
        bitfield.track_info.GENRE_NAME = 1;
    }
    
    /* Request to get the composer */
    if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_COMPOSER))
    {
        bitfield.track_info.COMPOSER_NAME = 1;
    }

    /* Request to get the release date */
    if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_RELEASE_DATE))
    {
        bitfield.track_info.LAST_PLAYED_DATA = 1;
    }
    
    /* Request to get the capability */
    if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY))
    {
        bitfield.track_info.CAPABILITIES = 1;
    }

    /* Request to get the track length */
    if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_TRACKLENGTH))
    {
        bitfield.track_info.DURATION = 1;
    }
    
    /* Request to get the chapter count */
    if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_CHAPTER_COUNT))
    {
        bitfield.track_info.CHAPTER_COUNT = 1;
    }

    /* Request to get the uid */
    if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_UID))
    {
        bitfield.track_info.UID = 1;
    }
    
    /* Request to get the track kind */
    if(is_setbit(*mask, IPOD_PLAYER_TRACK_INFO_MASK_TRACK_KIND))
    {
        /* todo */
        //rc = iPodCoreFuncGetTrackInfoType(devID, type, trackIndex, IPODCORE_GET_TRACK_INFO_COUNT_1, GENRE_NAME, callback);
        rc = IPOD_OK;
        *mask = *mask & (~IPOD_PLAYER_TRACK_INFO_MASK_TRACK_KIND);
    }
    
    if(bitfield.bitmask != 0)
    {
        rc = iPodCoreFuncGetTrackInfoType(devID, type, trackIndex, IPODCORE_GET_TRACK_INFO_COUNT_1, bitfield, callback);
    }

    if((*mask == 0) && (rc != IPOD_UNKNOWN_ID))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        rc = IPOD_ERROR;
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncGetTrackInfo(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackID, U32 *mask, U32 *curMask, U32 featureMask, IPOD_PLAYER_TRACK_INFO *info, void * callback)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    /* Parameter check */
    if(info == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, info);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    
    /* Connected iPod check */
    if((featureMask & IPOD_PLAYER_FEATURE_MASK_TRACK_INFO_RESTRICT) != IPOD_PLAYER_FEATURE_MASK_TRACK_INFO_RESTRICT)
    {
        /* Connected iPod cannot use getUID/PB/DBTrackInfo */
        /* Check the track type */
        if(type == IPOD_PLAYER_TRACK_TYPE_PLAYBACK)
        {
            /* Getting track type is playback */
            rc = iPodCoreFuncGetTrackInfoByPB(devID, type, (U32)trackID, mask, curMask, featureMask, info, (IPOD_CB_PLAYING_TRACK_INFO) callback);
        }
        else
        {
            /* Other type cannot use in connected iPod */
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, featureMask);
        }
    }
    else
    {
        /* Connected iPod can use getUID/PB/DBTrackInfo */
        /* Check the track type */
        if((type == IPOD_PLAYER_TRACK_TYPE_PLAYBACK) || (type == IPOD_PLAYER_TRACK_TYPE_DATABASE) | (type == IPOD_PLAYER_TRACK_TYPE_UID))
        {
            /* Get the track info  */
            rc = iPodCoreFuncGetTrackInfoByType(devID, type, trackID, mask, curMask, featureMask, info, (IPOD_TRACK_INFORMATION_CB)callback);
        }
        else
        {
            /* Selected type does not use */
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, type);
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncGetChapterInfoOld(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackID, U32 chapterIndex, U32 mask, U32 *curMask, IPOD_PLAYER_CHAPTER_INFO *info)
{
    S32 rc = IPOD_ERROR;
    U32 totalLen = 0;
    U32 curLen = 0;
    U32 trackIndex = 0;
    U8 *temp = NULL;
    
    /* For lint. In the future, ipod ctrl will be changed. then devID is used */
    type = type;
    
    /* Parameter check */
    if((curMask == NULL) || (info == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, curMask, info);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the current playing track index */
    rc = iPodGetCurrentPlayingTrackIndex(devID);
    if(rc >= 0)
    {
        /* Track Index is more than 0 */
        trackIndex = rc;
        
        rc = IPOD_OK;
        if(trackIndex != (U32)trackID)
        {
            /* Old iPod supports only current playing chapter info */
            rc = IPOD_BAD_PARAMETER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, trackIndex, trackID);
        }
    }
    else
    {
        IPOD_DLT_WARN("iPodGetCurrentPlayingTrackIndex :rc=%d", rc);
    }
    
    if(rc == IPOD_OK)
    {
        /* Selected mask is length */
        if(is_setbit(mask, IPOD_PLAYER_CHAPTER_INFO_MASK_LENGTH))
        {
            /* Get the current playing chapter status */
            rc = iPodGetCurrentPlayingTrackChapterPlayStatus(devID, chapterIndex, &totalLen, &curLen);
            if(rc == IPOD_OK)
            {
                info->length = totalLen;
                /* Set the mask */
                *curMask |= IPOD_PLAYER_CHAPTER_INFO_MASK_LENGTH;
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
            }
        }
        
        /* Selected mask is name */
        if(is_setbit(mask, IPOD_PLAYER_CHAPTER_INFO_MASK_NAME))
        {
            temp = calloc(iPodGetMaxPayloadSize(devID), sizeof(U8));
            if(temp != NULL)
            {
                /* Get the current playing chapter name */
                rc = iPodGetCurrentPlayingTrackChapterName(devID, chapterIndex, temp);
                if(rc == IPOD_OK)
                {
                    strncpy((char *)info->chapterName, (const char*)temp, IPOD_PLAYER_CHAPTER_NAME_LEN_MAX);
                    info->chapterName[IPOD_PLAYER_CHAPTER_NAME_LEN_MAX - 1] = '\0';
                    /* Set the mask */
                    *curMask |= IPOD_PLAYER_CHAPTER_INFO_MASK_NAME;
                }
                else
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
                }
                free(temp);
                temp = NULL;
            }
            else
            {
                rc = IPOD_ERR_NOMEM;
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
            }
        }
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncGetChapterInfoByType(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackID, U32 chapterIndex, U32 mask, U32 *curMask, IPOD_PLAYER_CHAPTER_INFO *info, IPOD_TRACK_INFORMATION_CB callback)
{
    S32 rc = IPOD_ERROR;
    
    /* For lint. In the future, ipod ctrl will be changed. then devID is used */
    chapterIndex = chapterIndex;

    /* Parameter check */
    if((curMask == NULL) || (info == NULL) || (callback == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, curMask, info, callback);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Selected mask is length */
    if(is_setbit(mask, IPOD_PLAYER_CHAPTER_INFO_MASK_LENGTH))
    {
        /* Check the track type */
        switch(type)
        {
            /* Track type is playback */
            case IPOD_PLAYER_TRACK_TYPE_PLAYBACK:
                /* Get the chapter information from playback */
                rc = iPodGetPBTrackInfo(devID, (U32)trackID, 1, CHAPTER_TIMES, callback);
                break;
                
            /* Track type is database */
            case IPOD_PLAYER_TRACK_TYPE_DATABASE:
                /* Get the chapter information from database */
                rc = iPodGetDBTrackInfo(devID, (U32)trackID, 1, CHAPTER_TIMES, callback);
                break;
                
            /* Track type is UID */
            case IPOD_PLAYER_TRACK_TYPE_UID:
                /* Get the chapter information from UID */
                rc = iPodGetUIDTrackInfo(devID, trackID, CHAPTER_TIMES, callback);
                break;
                
            /* Track type is unknown */
            default :
                /* Set the error */
                rc = IPOD_BAD_PARAMETER;
                break;
        }
        
    }
    
    /* Selected mask is name */
    if(is_setbit(mask, IPOD_PLAYER_CHAPTER_INFO_MASK_NAME))
    {
        /* Check the track type */
        switch(type)
        {
            /* Track type is playback  */
            case IPOD_PLAYER_TRACK_TYPE_PLAYBACK:
                /* Get the chapter information from playback */
                rc = iPodGetPBTrackInfo(devID, (U32)trackID, 1, CHAPTER_NAMES, callback);
                break;
                
            /* Track type is database */
            case IPOD_PLAYER_TRACK_TYPE_DATABASE:
                /* Get the chapter information from database */
                rc = iPodGetDBTrackInfo(devID, (U32)trackID, 1, CHAPTER_NAMES, callback);
                break;
                
            /* Track type is UID */
            case IPOD_PLAYER_TRACK_TYPE_UID:
                /* Get the chapter information from UID */
                rc = iPodGetUIDTrackInfo(devID, trackID, CHAPTER_NAMES, callback);
                break;
                
            /* Track type is unknown */
            default :
                /* Set the error */
                rc = IPOD_BAD_PARAMETER;
                break;
        }
    }
    
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


S32 iPodCoreFuncGetChapterInfo(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackID, U32 chapterIndex, U32 mask, U32 *curMask, U32 featureMask, IPOD_PLAYER_CHAPTER_INFO *info, void * callback)
{
    S32 rc = IPOD_PLAYER_OK;
    
    if(info == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, info);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    
    /* Connected iPod does not support getUID/PB/DBTrackInfo*/
    if((featureMask & IPOD_PLAYER_FEATURE_MASK_TRACK_INFO_RESTRICT) != IPOD_PLAYER_FEATURE_MASK_TRACK_INFO_RESTRICT)
    {
        /* Get the chapter info by old command */
        rc = iPodCoreFuncGetChapterInfoOld(devID, type, (U32)trackID, chapterIndex, mask, curMask, info);
    }
    
    /* Connected iPod supports getUID/PB/DBTrackInfo */
    else
    {
        /* Get the chapter info by new command */
        rc = iPodCoreFuncGetChapterInfoByType(devID, type, trackID, chapterIndex, mask, curMask, info, (IPOD_TRACK_INFORMATION_CB)callback);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncGetMode(U32 devID, U8 *mode)
{
    S32 rc = IPOD_ERROR;
    U8 iPodMode = 0;
    
    /* Parameter check */
    if(mode == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, mode);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the current mode by new command */
    rc = iPodGetUIMode(devID, &iPodMode);
    if(rc != IPOD_OK)
    {
        /* Get the current mode by old command */
        rc = iPodGetRemoteUIMode(devID);
        if(rc >= 0)
        {
            iPodMode = rc;
            rc = IPOD_OK;
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
    }
    
    if(rc == IPOD_OK)
    {
        /* Check the mode */
        switch(iPodMode)
        {
            /* Current mode is simple mode */
            case 0:
                /* Set the self control mode */
                *mode = IPOD_PLAYER_MODE_SELF_CONTROL;
                break;
                
            /* Current mode is extended mode */
            case 1:
                /* Set the remote control mode */
                *mode = IPOD_PLAYER_MODE_REMOTE_CONTROL;
                break;
                
            /* Current mode is iPodOut mode */
            case 2:
                /* Set the HMI Control mode */
                *mode = IPOD_PLAYER_MODE_HMI_CONTROL;
                break;
                
            /* Cunrrent mode is unknown */
            default:
                /* Set the unknown mode */
                *mode = IPOD_PLAYER_MODE_UNKNOWN;
                rc = IPOD_ERROR;
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc, iPodMode);
                break;
        }
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
    
}
S32 iPodCoreFuncGetRepeat(U32 devID, U8 *repeat)
{
    S32 rc = IPOD_ERROR;
    U8 iPodRepeat = 0;
    
    /* Paramter check */
    if(repeat == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, repeat);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the current repeat mode */
    rc = iPodGetRepeatMode(devID);
    if(rc >= 0)
    {
        /* Set the result */
        iPodRepeat = rc;
        rc = IPOD_OK;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    if(rc == IPOD_OK)
    {
        /* Check the current repeat mode */
        switch(iPodRepeat)
        {
            /* Curret mode is repeat off */
            case IPOD_REPEAT_OFF:
                *repeat = IPOD_PLAYER_REPEAT_OFF;
                break;
                
            /* Current mode is repeat one track */
            case IPOD_REPEAT_ONE_TRACK:
                *repeat = IPOD_PLAYER_REPEAT_ONE;
                break;
                
            /* Current mode is repeat all tracks */
            case IPOD_REPEAT_ALL_TRACKS:
                *repeat = IPOD_PLAYER_REPEAT_ALL;
                break;
            
            /* Current mode is unknown mode */
            default:
                *repeat = IPOD_PLAYER_REPEAT_UNKNOWN;
                rc = IPOD_ERROR;
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodRepeat);
                break;
        }
    }
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


S32 iPodCoreFuncGetShuffle(U32 devID, U8 *shuffle)
{
    S32 rc = IPOD_ERROR;
    U8 iPodShuffle = 0;
    
    /* Parameter check */
    if(shuffle == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, shuffle);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the current shuffle mode */
    rc = iPodGetShuffleMode(devID);
    if(rc >= 0)
    {
        iPodShuffle = rc;
        rc = IPOD_OK;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    if(rc == IPOD_OK)
    {
        /* Check the current shuffle mode */
        switch(iPodShuffle)
        {
            /* Curret mode is shuffle off */
            case IPOD_SHUFFLE_OFF:
                *shuffle = IPOD_PLAYER_SHUFFLE_OFF;
                break;
                
            /* Current mode is shuffle tracks */
            case IPOD_SHUFFLE_TRACKS:
                *shuffle = IPOD_PLAYER_SHUFFLE_TRACKS;
                break;
                
            /* Current mode is shuffle album */
            case IPOD_SHUFFLE_ALBUMS:
                *shuffle = IPOD_PLAYER_SHUFFLE_ALBUMS;
                break;
                
            /* Current mode is unknown */
            default:
                *shuffle = IPOD_PLAYER_SHUFFLE_STATUS_UNKNOWN;
                rc = IPOD_ERROR;
                break;
        }
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


S32 iPodCoreFuncGetSpeed(U32 devID, U8 *speed)
{
    S32 rc = IPOD_ERROR;
    U8 iPodSpeed = 0;
    
    /* Paramtere check */
    if(speed == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, speed);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the current audiobook speed */
    rc = iPodGetAudioBookSpeed(devID);
    if(rc >= 0)
    {
        iPodSpeed = rc;
        rc = IPOD_OK;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    if(rc == IPOD_OK)
    {
        /* Check the current audiobook speed */
        switch(iPodSpeed)
        {
            /* Current speed is normal */
            case IPOD_AUDIOBOOK_SPEED_NORMAL:
                *speed = IPOD_PLAYER_PLAYING_SPEED_NORMAL;
                break;
                
            /* Current speed is fast */
            case IPOD_AUDIOBOOK_SPEED_FAST:
                *speed = IPOD_PLAYER_PLAYING_SPEED_FAST;
                break;
                
            /* Current speed is slow */
            case IPOD_AUDIOBOOK_SPEED_SLOW:
                *speed = IPOD_PLAYER_PLAYING_SPEED_SLOW;
                break;
                
            /* Current speed is unknown */
            default:
                *speed = IPOD_PLAYER_PLAYING_SPEED_UNKNOWN;
                rc = IPOD_ERROR;
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodSpeed);
                break;
        }
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


S32 iPodCoreFuncGetTrackTotalCount(U32 devID, const IPOD_PLAYER_TRACK_TYPE type, U32 *count)
{
    S32 rc = IPOD_ERROR;
    
    /* Parameter check */
    if(count == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, count);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    
    /* Check the track type */
    switch(type)
    {
        /* Track type is playback */
        case IPOD_PLAYER_TRACK_TYPE_PLAYBACK:
            /* Get the track total count from playback */
            rc = iPodGetNumPlayingTracks(devID);
            break;
            
        /* Track type is database */
        case IPOD_PLAYER_TRACK_TYPE_DATABASE:
            /* Get the track total count from database */
            rc = iPodGetNumberCategorizedDBRecords(devID, IPOD_CAT_TRACK);
            break;
            
        /* Track type is UID or unknown */
        default:
            rc = IPOD_BAD_PARAMETER;
            break;
    }
    
    if(rc >= 0)
    {
        *count = rc;
        rc = IPOD_OK;
    }
    else
    {
        *count = 0;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncGetEqualizer(U32 devID, U32 *equalizer)
{
    S32 rc = IPOD_ERROR;
    
    /* Parameter check */
    if(equalizer == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, equalizer);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get current equalizer setting */
    rc = iPodGetCurrentEQProfileIndex(devID, equalizer);
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}
S32 iPodCoreFuncGetEqualizerName(U32 devID, const U8 equalizer, U8 *name)
{
    S32 rc = IPOD_ERROR;
    
    /* Parameter check */
    if(name == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, name);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the current equalizer name */
    rc = iPodGetIndexedEQProfileName(devID, (U32)equalizer, name, IPOD_PLAYER_STRING_LEN_MAX);
    if(rc != IPOD_OK)
    {
        name[0] = '\0';
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncGetRunningApp(U32 devID, U8 *name, U16 length)
{
    S32 rc = IPOD_ERROR;
    
    /* Parameter check */
    if(name == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, name);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    
    /* Get the current playing application */
    rc = iPodGetNowPlayingFocusApp(devID, name, length);
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


static S32 iPodCoreChangeCategoryToiPod(IPOD_PLAYER_DB_TYPE type, IPOD_CATEGORY *category)
{
    S32 rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
    U32 i = 0;
    U32 tblsize =0;
    
    tblsize = sizeof(ipodcore_category_mapping_tbl)/sizeof(ipodcore_category_mapping_tbl[0]);
    
    /* table search */
    for(i = 0; i < tblsize; i++){
        /* category is type of player */
        if(type == ipodcore_category_mapping_tbl[i].player_category){
            /* change it to type of ipod  */
            *category = ipodcore_category_mapping_tbl[i].ctrl_category;
            rc = IPOD_PLAYER_OK;
            break;
        }
    }
    
    if(rc != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, i, tblsize);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


S32 iPodCoreFuncGetLowerCatList(U32 devID, IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_CATEGORY category = IPOD_CAT_PLAYLIST;
    U32 i = 0;
    
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Clear the number of categories */
    memset(iPodCtrlCfg->iPodInfo->catCountList, 0, sizeof(iPodCtrlCfg->iPodInfo->catCountList));
    
    for(i = 0; i < IPOD_PLAYER_DB_TYPE_ITUNESU + 1; i++)
    {
        /* Change the category type from iPodPlayer to iPodCtrl */
        rc = iPodCoreChangeCategoryToiPod((IPOD_PLAYER_DB_TYPE)i, &category);
        if(rc == IPOD_PLAYER_OK)
        {
            /* Genius mixes category has bug if version is iOS3.XX */
            /* To avoid this bug, if category is genius and version is iOS3.XX, number of category is not requested */
            if(!((category == IPOD_CAT_GENIUS) && (iPodCtrlCfg->iPodInfo->property.softVer.majorVer == IPOD_PLAYER_GENIUS_NO_GET_VER)))
            {
                /* Get the number of records */
                rc = iPodGetNumberCategorizedDBRecords(devID, category);
                if(rc >= 0)
                {
                    iPodCtrlCfg->iPodInfo->catCountList[i] = rc;
                }
            }
            rc = IPOD_PLAYER_OK;
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncGetDBCount(U32 devID, IPOD_PLAYER_DB_TYPE type, U32 *num)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_CATEGORY category = IPOD_CAT_PLAYLIST;
    
    /* Parameter check */
    if(num == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, num);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    
    /* Change the category type from iPodPlayer to iPodCtrl */
    rc = iPodCoreChangeCategoryToiPod(type, &category);
    if(rc == IPOD_PLAYER_OK)
    {
        /* Get the number of records */
        rc = iPodGetNumberCategorizedDBRecords(devID, category);
        if(rc >= 0)
        {
            *num = rc;
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
            rc = IPOD_PLAYER_ERR_IPOD_CTRL_ERROR;
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


S32 iPodCoreFuncSelectDBEntry(U32 devID, IPOD_PLAYER_DB_TYPE type, S32 entry)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_CATEGORY category = IPOD_CAT_PLAYLIST;
    
    
    /* Change the category type from iPodPlayer to iPodCtrl */
    rc = iPodCoreChangeCategoryToiPod(type, &category);
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* SelectDBRecord */
        rc = iPodSelectDBRecord(devID, category, (U32)entry);
        if(rc != IPOD_OK)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
        
        /* Change return code of iPod ctrl to player */
        rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncGetDBEntries(U32 devID, IPOD_PLAYER_DB_TYPE type, U32 start, S32 num, const IPOD_CB_RETRIEVE_CAT_DB_RECORDS callback)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    IPOD_CATEGORY category = IPOD_CAT_PLAYLIST;
    
    
    /* Change the category type from iPodPlayer to iPodCtrl */
    rc = iPodCoreChangeCategoryToiPod(type, &category);
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodRetrieveCategorizedDBRecords(devID, category, start, num, callback);
        if(rc != IPOD_OK)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
        
        /* Change return code of iPod ctrl to player */
        rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncCancel(U32 devID, IPOD_PLAYER_CANCEL_TYPE type)
{
    S32 rc = IPOD_ERROR;
    IPOD_CANCEL_COMMAND_TYPE cancelType = IPOD_REMOTE_ARTWORK_DATA;
    
    
    switch(type)
    {
        case IPOD_PLAYER_CANCEL_COVERART:
            cancelType = IPOD_EXTEND_ARTWORK_DATA;
            rc = IPOD_OK;
            break;
            
        case IPOD_PLAYER_CANCEL_DB_ENTRY:
            cancelType = IPOD_RETRIEVE_CATEGORIZED_DB_RECORDS;
            rc = IPOD_OK;
            break;
            
        default:
            rc = IPOD_BAD_PARAMETER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, type);
            break;
    }
    
    if(rc == IPOD_OK)
    {
        rc = iPodCancelCommand(devID, cancelType);
        if(rc != IPOD_OK)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
    }
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncClearSelection(U32 devID, IPOD_PLAYER_DB_TYPE type, IPOD_PLAYER_DB_TYPE topType, IPOD_PLAYER_DB_TYPE curType)
{
    S32 rc = IPOD_ERROR;
    
    
    /* Check tye category type */
    switch(type)
    {
        /* Category is All */
        case IPOD_PLAYER_DB_TYPE_ALL:
            /* Back to Top category */
            rc = iPodResetDBSelection(devID);
            break;
            
        /* Category is Playlist */
        case IPOD_PLAYER_DB_TYPE_PLAYLIST:
            /* Back to Top category */
            rc = iPodResetDBSelection(devID);
            break;
            
        /* Category is All */
        case IPOD_PLAYER_DB_TYPE_ARTIST:
            /* Check the Top category type */
            switch(topType)
            {
                /* Top category of current selected category is Artist */
                case IPOD_PLAYER_DB_TYPE_ARTIST:
                /* Top category of current selected category is Genre */
                case IPOD_PLAYER_DB_TYPE_GENRE:
                    /* Check the current category type */
                    if(curType == IPOD_PLAYER_DB_TYPE_TRACK)
                    {
                        /* Category type Track is equal to current type is Album */
                        curType = IPOD_PLAYER_DB_TYPE_ALBUM;
                    }
                    
                    if(curType == IPOD_PLAYER_DB_TYPE_ALBUM)
                    {
                        /* Back to previous category. It means Artist*/
                        rc = iPodSelectDBRecord(devID, IPOD_CAT_ALBUM, (U32)-1);
                        if(rc == IPOD_OK)
                        {
                            curType = IPOD_PLAYER_DB_TYPE_ARTIST;
                        }
                    }
                    
                    /* Current category is higher than Artist category */
                    if((rc == IPOD_OK) && (curType != IPOD_PLAYER_DB_TYPE_ARTIST))
                    {
                        /* Back to Top category */
                        rc = iPodResetDBSelection(devID);
                    }
                    break;
                    
                default:
                    /* Back to Top category */
                    rc = iPodResetDBSelection(devID);
                    break;
            }
            break;
            
        /* Category is Album */
        case IPOD_PLAYER_DB_TYPE_ALBUM:
            /* Check the Top category type */
            switch(topType)
            {
                /* Type is Artist */
                case IPOD_PLAYER_DB_TYPE_ARTIST:
                /* Type is Genre */
                case IPOD_PLAYER_DB_TYPE_GENRE:
                /* Type is Album */
                case IPOD_PLAYER_DB_TYPE_ALBUM:
                    /* Check the current type */
                    if(curType == IPOD_PLAYER_DB_TYPE_TRACK)
                    {
                        /* Type is track. It means type album */
                        curType = IPOD_PLAYER_DB_TYPE_ALBUM;
                    }
                    
                    if(curType != IPOD_PLAYER_DB_TYPE_ALBUM)
                    {
                        /* Back to Top category */
                        rc = iPodResetDBSelection(devID);
                    }
                    break;
                default:
                    /* Back to Top category */
                    rc = iPodResetDBSelection(devID);
                    break;
            }
            break;
            
        /* Category is Genre */
        case IPOD_PLAYER_DB_TYPE_GENRE:
            /* Check the Top category type */
            switch(topType)
            {
                /* Type is Genre */
                case IPOD_PLAYER_DB_TYPE_GENRE:
                
                    /* Type is track. It means type is Album */
                    if(curType == IPOD_PLAYER_DB_TYPE_TRACK)
                    {
                        curType = IPOD_PLAYER_DB_TYPE_ALBUM;
                    }
                    
                    if(curType == IPOD_PLAYER_DB_TYPE_ALBUM)
                    {
                        /* Back to previous type. It means Artist */
                        rc = iPodSelectDBRecord(devID, IPOD_CAT_ALBUM, (U32)-1);
                        if(rc == IPOD_OK)
                        {
                            curType = IPOD_PLAYER_DB_TYPE_ARTIST;
                        }
                    }
                    
                    if(curType == IPOD_PLAYER_DB_TYPE_ARTIST)
                    {
                        /* Back to previous type. It means Genre */
                        rc = iPodSelectDBRecord(devID, IPOD_CAT_ARTIST, (U32)-1);
                        if(rc == IPOD_OK)
                        {
                            curType = IPOD_PLAYER_DB_TYPE_GENRE;
                        }
                    }
                    
                    /* category is higher than Genre */
                    if((rc == IPOD_OK) && (curType != IPOD_PLAYER_DB_TYPE_GENRE))
                    {
                        /* Back to Top category */
                        rc = iPodResetDBSelection(devID);
                    }
                    break;
                
                default:
                    /* Back to Top category */
                    rc = iPodResetDBSelection(devID);
                    break;
            }
            break;

        /* Category is Track */
        case IPOD_PLAYER_DB_TYPE_TRACK:
            rc = iPodResetDBSelection(devID);
            break;
            
        /* Category is Composer */
        case IPOD_PLAYER_DB_TYPE_COMPOSER:
            rc = iPodResetDBSelection(devID);
            break;
            
        /* Category is Audiobook */
        case IPOD_PLAYER_DB_TYPE_AUDIOBOOK:
            rc = iPodResetDBSelection(devID);
            break;
            
        /* Category is Podcast */
        case IPOD_PLAYER_DB_TYPE_PODCAST:
            rc = iPodResetDBSelection(devID);
            break;
            
        /* Category is Nested Playlist */
        case IPOD_PLAYER_DB_TYPE_NESTED_PLAYLIST:
            rc = iPodResetDBSelection(devID);
            break;
            
        /* Category is Genius Playlist */
        case IPOD_PLAYER_DB_TYPE_INTELLIGENT:
            rc = iPodResetDBSelection(devID);
            break;
            
        /* Category is iTunes U */
        case IPOD_PLAYER_DB_TYPE_ITUNESU:
            rc = iPodResetDBSelection(devID);
            break;
            
        /* Category is Unknown */
        default:
            rc = IPOD_BAD_PARAMETER;
            break;
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    if(rc != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncSelectAV(U32 devID, U8 avType)
{
    S32 rc = IPOD_ERROR;
    U8 selection = 0;
    
    
    /* Check the selected type */
    switch(avType)
    {
        /* Selected type is audio */
        case IPOD_PLAYER_AUDIO_MODE:
            selection = 1;
            rc = IPOD_OK;
            break;
            
        /* Selected type is video */
        case IPOD_PLAYER_VIDEO_MODE:
            selection = 2;
            rc = IPOD_OK;
            break;
            
        /* Selected type is unknown */
        default:
            rc = IPOD_BAD_PARAMETER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, avType);
            break;
    }
    
    if(rc == IPOD_OK)
    {
        /* Reset the database hierarchy */
        rc = iPodResetDBSelectionHierarchy(devID, selection);
        if(rc != IPOD_OK)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
    }
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


S32 iPodCoreFuncSendToApp(U32 devID, U32 handle, U32 dataSize, U8 *appName)
{
    S32 rc = IPOD_ERROR;
    
    
    rc = iPodDevDataTransfer(devID, (U16)handle, appName, dataSize);
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


S32 iPodCoreFuncRequestAppStart(U32 devID, U8 *appName)
{
    S32 rc = IPOD_ERROR;
    U16 length = 0;
    
    /* Parameter check */
    if(appName == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, appName);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the length of application name */
    length = strnlen((char *)appName, IPOD_PLAYER_STRING_LEN_MAX) + 1;
    
    /* Check the length whether length is more than maximum length or not */
    if(length >= IPOD_PLAYER_STRING_LEN_MAX)
    {
        appName[IPOD_PLAYER_STRING_LEN_MAX - 1] = '\0';
        length = IPOD_PLAYER_STRING_LEN_MAX;
    }
    
    /* Start application */
    rc = iPodRequestAppLaunch(devID, appName, (U8)length);
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

static S32 iPodCoreFuncAddXMLNode(xmlTextWriterPtr xmlText, U8 *key, const U8 *value, U8 *attr, U8 *attrValue, BOOL endNode)
{
    S32 rc = -1;                    /* error code of mxl */
    
    /* Parameter check */
    if((xmlText == NULL) || (key == NULL) || ((value == NULL) && (endNode == TRUE)))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, xmlText, key, value, endNode);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Start the Element */
    rc = xmlTextWriterStartElement(xmlText, BAD_CAST key);
    if(rc >= 0)
    {
        /* Check the paramter */
        if((attr != NULL) && (attrValue != NULL))
        {
            /* Add the attribute and value */
            rc = xmlTextWriterWriteAttribute(xmlText, BAD_CAST attr, BAD_CAST attrValue);
        }
        
        if((value != NULL) && (rc >= 0))
        {
            rc = xmlTextWriterWriteString(xmlText, BAD_CAST value);
        }
    }
    
    if((endNode == TRUE) && (rc >= 0))
    {
        /* End the element */
        rc = xmlTextWriterEndElement(xmlText);
    }
    
    if(rc >= 0)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        rc = IPOD_PLAYER_ERROR;
    }
    
    return rc;
}

static S32 iPodCoreFuncAddXMLNodeBase64(xmlTextWriterPtr xmlText, U8 *key, const U8 *value, U8 *attr, U8 *attrValue, BOOL endNode)
{
    S32 rc = -1;                    /* error code of mxl */
    S32 pos = 0;
    size_t length = 0;

    /* Parameter check */
    if((xmlText == NULL) || (key == NULL) || ((value == NULL) && (endNode == TRUE)))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, xmlText, key, value, endNode);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Start the Element */
    rc = xmlTextWriterStartElement(xmlText, BAD_CAST key);
    if(rc >= 0)
    {
        /* Check the paramter */
        if((attr != NULL) && (attrValue != NULL))
        {
            /* Add the attribute and value */
            rc = xmlTextWriterWriteAttribute(xmlText, BAD_CAST attr, BAD_CAST attrValue);
        }
    }
    
    if((value != NULL) && (rc >= 0))
    {
        length = strlen((const char *)value);
        /* xmlTextWriterWriteBase64 inserts CR-LF when input length is bigger than internal defined value */
        while(length > BASE64_ENCODE_INPUT_CHUNK_SIZE)
        {
            /* Add the unknown data with base64 */
            rc = xmlTextWriterWriteBase64(xmlText, (const char *)value, pos, BASE64_ENCODE_INPUT_CHUNK_SIZE);
            length -= BASE64_ENCODE_INPUT_CHUNK_SIZE;
            pos += BASE64_ENCODE_INPUT_CHUNK_SIZE;
        }
        if(length > 0)
        {
            /* Add the unknown data with base64 */
            rc = xmlTextWriterWriteBase64(xmlText, (const char *)value, pos, length);
        }
    }
    
    if((endNode == TRUE) && (rc >= 0))
    {
        /* End the element */
        rc = xmlTextWriterEndElement(xmlText);
    }
    
    if(rc >= 0)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        rc = IPOD_PLAYER_ERROR;
    }
    
    return rc;
}

static S32 iPodCoreFuncAddKeyAndValue(xmlTextWriterPtr xmlText, U8 *valueTag, const U8 *keyName, const U8 *value)
{
    S32 rc = IPOD_PLAYER_OK;
    
    if(xmlText == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, xmlText);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Add the node of "key" */
    rc = iPodCoreFuncAddXMLNode(xmlText, (U8 *)IPODCORE_TAG_NODE_KEY, keyName, NULL, NULL, TRUE);
    if(rc == IPOD_PLAYER_OK)
    {
        /* Add the node of keyvalue */
        rc = iPodCoreFuncAddXMLNode(xmlText, valueTag, value, NULL, NULL, TRUE);
    }
    
    return rc;
}

static S32 iPodCoreFuncAddFixedElements(xmlTextWriterPtr xmlText)
{
    S32 rc = -1;                    /* error code of mxl */
    U8 tagMajor[IPOD_PLAYER_CFG_STR_MAX_SIZE] = {0};
    U8 tagMinor[IPOD_PLAYER_CFG_STR_MAX_SIZE] = {0};
    U8 tagManid[IPOD_PLAYER_CFG_STR_MAX_SIZE] = {0};
    U8 tagManName[IPOD_PLAYER_CFG_STR_MAX_SIZE] = {0};
    U8 tagDevName[IPOD_PLAYER_CFG_STR_MAX_SIZE] = {0};
    U8 tagMarked[IPOD_PLAYER_CFG_STR_MAX_SIZE] = {0};
    
    
    if(xmlText == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, xmlText);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Add the DTD to XML */
    rc = xmlTextWriterWriteDTD(xmlText, (const xmlChar *)IPODCORE_TAG_DTD_NAME, (const xmlChar *)IPODCORE_TAG_DTD_PUBLIC, (const xmlChar *)IPODCORE_TAG_DTD_SYSTEM, NULL);
    if(rc >= 0)
    {
        /* start a new line */
        rc = xmlTextWriterWriteString(xmlText, BAD_CAST "\n");
        
    }
    
    if(rc >= 0)
    {
        /* Set the 1 indent to XML */
        rc = xmlTextWriterSetIndent(xmlText, 1);
    }
    
    if(rc >= 0)
    {
        /* Set the 4 space to XML */
        rc = xmlTextWriterSetIndentString(xmlText, BAD_CAST IPODCORE_TAG_INDENT_SPACE);
    }
    
    if(rc >= 0)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        rc = IPOD_PLAYER_ERROR;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Add the element of "plist" to XML. */
        rc = iPodCoreFuncAddXMLNode(xmlText, (U8 *)IPODCORE_TAG_NODE_PLIST, NULL, (U8 *)IPODCORE_TAG_ATTR_VER, (U8 *)IPODCORE_TAG_VER_DATA, FALSE);
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Add the element of "dict"t */
        rc = iPodCoreFuncAddXMLNode(xmlText, (U8 *)IPODCORE_TAG_NODE_DICT, NULL, NULL, NULL, FALSE);
    }

    if(rc == IPOD_PLAYER_OK)
    {
        /* Add the element of "major" and value */
        rc = iPodCoreGetCfs(IPOD_PLAYER_CFGNUM_TAG_MAJOR, sizeof(tagMajor), tagMajor);
        if(rc == IPOD_PLAYER_OK)
        {
            rc = iPodCoreFuncAddKeyAndValue(xmlText, (U8 *)IPODCORE_TAG_NODE_INT, (U8 *)IPODCORE_TAG_NODE_MAJOR, tagMajor);
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Add the element of "minor" and value */
        rc = iPodCoreGetCfs(IPOD_PLAYER_CFGNUM_TAG_MINOR, sizeof(tagMinor), tagMinor);
        if(rc == IPOD_PLAYER_OK)
        {
            rc = iPodCoreFuncAddKeyAndValue(xmlText, (U8 *)IPODCORE_TAG_NODE_INT, (U8 *)IPODCORE_TAG_NODE_MINOR, tagMinor);
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Add the element of "manufacture id" and value */
        rc = iPodCoreGetCfs(IPOD_PLAYER_CFGNUM_TAG_MANID, sizeof(tagManid), tagManid);
        if(rc == IPOD_PLAYER_OK)
        {
            rc = iPodCoreFuncAddKeyAndValue(xmlText, (U8 *)IPODCORE_TAG_NODE_INT, (U8 *)IPODCORE_TAG_NODE_MANID, tagManid);
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Add the element of "manufacture name" and value */
        rc = iPodCoreGetCfs(IPOD_PLAYER_CFGNUM_TAG_MANNAME, sizeof(tagManName), tagManName);
        if(rc == IPOD_PLAYER_OK)
        {
            rc = iPodCoreFuncAddKeyAndValue(xmlText, (U8 *)IPODCORE_TAG_NODE_STR, (U8 *)IPODCORE_TAG_NODE_MANNAME, tagManName);
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Add the element of "device name" and value */
        rc = iPodCoreGetCfs(IPOD_PLAYER_CFGNUM_TAG_DEVNAME, sizeof(tagDevName), tagDevName);
        if(rc == IPOD_PLAYER_OK)
        {
            rc = iPodCoreFuncAddKeyAndValue(xmlText, (U8 *)IPODCORE_TAG_NODE_STR, (U8 *)IPODCORE_TAG_NODE_DEVNAME, tagDevName);
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Add the element of "marked track" */
        rc = iPodCoreGetCfs(IPOD_PLAYER_CFGNUM_TAG_MARKED, sizeof(tagMarked), tagMarked);
        if(rc == IPOD_PLAYER_OK)
        {
            rc = iPodCoreFuncAddXMLNode(xmlText, (U8 *)IPODCORE_TAG_NODE_KEY, tagMarked, NULL, NULL, TRUE);
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Add the "array" element */
        rc = iPodCoreFuncAddXMLNode(xmlText, (U8 *)IPODCORE_TAG_NODE_ARRAY, NULL, NULL, NULL, FALSE);
    }
    
    return rc;
}

S32 iPodCoreFuncSetSongTag(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, const IPOD_PLAYER_TAG_INFO *info)
{
    S32 rc = IPOD_PLAYER_ERROR;
    xmlTextWriterPtr xmlText= NULL;
    U8 unknownTemp[IPOD_PLAYER_TAG_STRING_LEN] = {0};
    S32 outLen = 0;
    S32 inLen = 0;

    if((iPodCtrlCfg == NULL) || (info == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, info);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(iPodCtrlCfg->xmlText == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg->xmlText);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    xmlText = iPodCtrlCfg->xmlText;
    
    /* Add the element of "dict" */
    rc = iPodCoreFuncAddXMLNode(xmlText, (U8 *)IPODCORE_TAG_NODE_DICT, NULL, NULL, NULL, FALSE);
    
    if((rc == IPOD_PLAYER_OK) && (info->ambiguous[0] != '\0'))
    {
        /* Add the element of "ambiguous" and add the value */
        rc = iPodCoreFuncAddKeyAndValue(xmlText, (U8 *)IPODCORE_TAG_NODE_INT, (U8 *)IPODCORE_TAG_NODE_AMBIGUOUS, info->ambiguous);
    }
    
    /* Add the elements of "button pressed" and add the value */
    if((rc == IPOD_PLAYER_OK) && (info->buttonPressed[0] != '\0'))
    {
        rc = iPodCoreFuncAddKeyAndValue(xmlText, (U8 *)IPODCORE_TAG_NODE_INT, (U8 *)IPODCORE_TAG_NODE_BUTTONPRESS, info->buttonPressed);
    }
    
    if((rc == IPOD_PLAYER_OK) && (info->name[0] != '\0'))
    {
        /* Add the element of "name" and value */
        rc = iPodCoreFuncAddKeyAndValue(xmlText, (U8 *)IPODCORE_TAG_NODE_STR, (U8 *)IPODCORE_TAG_NODE_NAME, info->name);
    }
    
    if((rc == IPOD_PLAYER_OK) && (info->artist[0] != '\0'))
    {
        /* Add the element of "artist" and value */
        rc = iPodCoreFuncAddKeyAndValue(xmlText, (U8 *)IPODCORE_TAG_NODE_STR, (U8 *)IPODCORE_TAG_NODE_ARTIST, info->artist);
    }
    
    if((rc == IPOD_PLAYER_OK) && (info->album[0] != '\0'))
    {
        /* Add the element of "album" and value */
        rc = iPodCoreFuncAddKeyAndValue(xmlText, (U8 *)IPODCORE_TAG_NODE_STR, (U8 *)IPODCORE_TAG_NODE_ALBUM, info->album);
    }
    
    if((rc == IPOD_PLAYER_OK) && (info->songID[0] != '\0'))
    {
        /* Add the element of "songid" and value */
        rc = iPodCoreFuncAddKeyAndValue(xmlText, (U8 *)IPODCORE_TAG_NODE_INT, (U8 *)IPODCORE_TAG_NODE_SONGID, info->songID);
    }
    
    if((rc == IPOD_PLAYER_OK) && (info->storefrontID[0] != '\0'))
    {
        /* Add element of "songfrontid" and value */
        rc = iPodCoreFuncAddKeyAndValue(xmlText, (U8 *)IPODCORE_TAG_NODE_INT, (U8 *)IPODCORE_TAG_NODE_FRONTID, info->storefrontID);
    }
    
    if((rc == IPOD_PLAYER_OK) && (info->frequency[0] != '\0'))
    {
        /* Add the element of "frequency" and value */
        rc = iPodCoreFuncAddKeyAndValue(xmlText, (U8 *)IPODCORE_TAG_NODE_STR, (U8 *)IPODCORE_TAG_NODE_FREQ, info->frequency);
    }
    
    if((rc == IPOD_PLAYER_OK) && (info->callLetter[0] != '\0'))
    {
        /* Add the element of "stationcallletters" and value */
        rc = iPodCoreFuncAddKeyAndValue(xmlText, (U8 *)IPODCORE_TAG_NODE_STR, (U8 *)IPODCORE_TAG_NODE_LETTERS, info->callLetter);
    }
    
    if((rc == IPOD_PLAYER_OK) && (info->feedURL[0] != '\0'))
    {
        /* Add the element of "podcastfeedurl" and value */
        rc = iPodCoreFuncAddKeyAndValue(xmlText, (U8 *)IPODCORE_TAG_NODE_INT, (U8 *)IPODCORE_TAG_NODE_FEEDURL, info->feedURL);
    }
    
    if((rc == IPOD_PLAYER_OK) && (info->stationURL[0] != '\0'))
    {
        /* Add the element of "stationurl" and value */
        rc = iPodCoreFuncAddKeyAndValue(xmlText, (U8 *)IPODCORE_TAG_NODE_STR, (U8 *)IPODCORE_TAG_NODE_STATIONURL, info->stationURL);
    }
    
    if((rc == IPOD_PLAYER_OK) && (info->genre[0] != '\0'))
    {
        /* Add the element of "genre" and value */
        rc = iPodCoreFuncAddKeyAndValue(xmlText, (U8 *)IPODCORE_TAG_NODE_STR, (U8 *)IPODCORE_TAG_NODE_GENRE, info->genre);
    }
    
    if((rc == IPOD_PLAYER_OK) && (info->timeStamp[0] != '\0'))
    {
        /* Add the element of "timestamp" and value */
        rc = iPodCoreFuncAddKeyAndValue(xmlText, (U8 *)IPODCORE_TAG_NODE_DATE, (U8 *)IPODCORE_TAG_NODE_TIMESTAMP, info->timeStamp);
    }
    
    if((rc == IPOD_PLAYER_OK) && (info->programNumber[0] != '\0'))
    {
        /* Add the element of "programnumber" and value */
        rc = iPodCoreFuncAddKeyAndValue(xmlText, (U8 *)IPODCORE_TAG_NODE_INT, (U8 *)IPODCORE_TAG_NODE_PROGRAM, info->programNumber);
    }
    
    if((rc == IPOD_PLAYER_OK) && (info->softStationID[0] != '\0'))
    {
        /* Add the element of "itunesstationid" and value */
        rc = iPodCoreFuncAddKeyAndValue(xmlText, (U8 *)IPODCORE_TAG_NODE_STR, (U8 *)IPODCORE_TAG_NODE_STATIONID, info->softStationID);
    }
    
    if((rc == IPOD_PLAYER_OK) && (info->affiliateID[0] != '\0'))
    {
        /* Add the element of "itunesaffiliateid" and value */
        rc = iPodCoreFuncAddKeyAndValue(xmlText, (U8 *)IPODCORE_TAG_NODE_STR, (U8 *)IPODCORE_TAG_NODE_AFFILIATE, info->affiliateID);
    }
    
    if((rc == IPOD_PLAYER_OK) && (info->unknownData[0] != '\0'))
    {
        rc = iPodCoreFuncAddXMLNode(xmlText, (U8 *)IPODCORE_TAG_NODE_KEY, (U8 *)IPODCORE_TAG_NODE_UNKNOWN, NULL, NULL, TRUE);
        if(rc == IPOD_PLAYER_OK)
        {
            outLen = sizeof(unknownTemp);
            inLen = strnlen((const char*)info->unknownData, sizeof(info->unknownData));
            /* UTF8 to ISO8859-1 */
            rc = UTF8Toisolat1(unknownTemp, &outLen, info->unknownData, &inLen);
            if(rc >= 0)
            {
                /* Add the element of "unknowndata" and value */
                rc = iPodCoreFuncAddXMLNodeBase64(xmlText, (U8 *)IPODCORE_TAG_NODE_DATA, unknownTemp, NULL, NULL, TRUE);
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
                rc = IPOD_PLAYER_ERROR;
            }
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* End the element of "dict" */
        rc = xmlTextWriterEndElement(xmlText);
        if(rc >= 0)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncOpenSongTag(U32 devID, IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 optionsBit, U32 optionLen, const U8 *optionData, U32 *tagHandle)
{
    S32 rc = IPOD_PLAYER_OK;
    IPOD_FILE_OPTIONS_MASK tagMask;
    IPOD_FEATURE_TYPE type = IPOD_RADIO_TAGGING;
    
    /* Parameter check */
    if((iPodCtrlCfg == NULL) || (tagHandle == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, tagHandle);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Parameter check */
    if(iPodCtrlCfg->xmlBuf != NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg->xmlBuf);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    
    /* Initialize the structure */
    memset(&tagMask, 0, sizeof(tagMask));
    
    /* Open the tag with binaly */
    if((optionsBit & IPOD_PLAYER_SONG_TAG_OPTIONS_BINALY) == IPOD_PLAYER_SONG_TAG_OPTIONS_BINALY)
    {
        tagMask.binary = 1;
        /* Parameter check */
        if(optionData == NULL)
        {
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
    }
    
    /* Open the tag with device information */
    if((optionsBit & IPOD_PLAYER_SONG_TAG_OPTIONS_DEVICE_INFO) == IPOD_PLAYER_SONG_TAG_OPTIONS_DEVICE_INFO)
    {
        tagMask.iPodInfoXML = 1;
    }
    
    /* Open the tag with signature */
    if((optionsBit & IPOD_PLAYER_SONG_TAG_OPTIONS_SIGNATURE) == IPOD_PLAYER_SONG_TAG_OPTIONS_SIGNATURE)
    {
        if(tagMask.iPodInfoXML == 1)
        {
            tagMask.signatureXML = 1;
        }
        else
        {
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, tagMask.iPodInfoXML);
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Open the tag file */
        rc = iPodOpeniPodFeatureFile(devID, type, &tagMask, optionData, (U8)optionLen, (U8 *)tagHandle);
        /* Change return code of iPod ctrl to player */
        rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Create a XML buffer, to which the XML document will be written */
        iPodCtrlCfg->xmlBuf = xmlBufferCreate();
        if(iPodCtrlCfg->xmlBuf == NULL)
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            /* Create a XMLWriter for memory. */
            iPodCtrlCfg->xmlText = xmlNewTextWriterMemory(iPodCtrlCfg->xmlBuf, 0);
            if (iPodCtrlCfg->xmlText == NULL)
            {
                rc = IPOD_PLAYER_ERR_NOMEM;
            }
            else
            {
                /* Start the XML documentation with UTF-8 */
                rc = xmlTextWriterStartDocument(iPodCtrlCfg->xmlText, NULL, IPODCORE_TAG_STR_UTF, NULL);
                if(rc >= 0)
                {
                    rc = IPOD_PLAYER_OK;
                }
                else
                {
                    rc = IPOD_PLAYER_ERROR;
                }
            }
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            /* Add the fixed element for tagging */
            rc = iPodCoreFuncAddFixedElements(iPodCtrlCfg->xmlText);
        }
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncWriteTagging(U32 devID, IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 tagHandle)
{
    S32 rc = -1;                    /* error code of mxl */
    U8 *tagData = NULL;
    U32 maxTagSize = 0;
    U32 writeLen = 0;
    
    /* Parameter check */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Paramter check */
    if(iPodCtrlCfg->xmlBuf == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg->xmlBuf);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    
    /* Write tag position is 0. It means that first write to tag file  */
    if(iPodCtrlCfg->curTagPos == 0)
    {
        if(iPodCtrlCfg->xmlText != NULL)
        {
            /* End an xml document. All open elements are closed */
            rc = xmlTextWriterEndDocument(iPodCtrlCfg->xmlText);
            if(rc >= 0)
            {
                rc = IPOD_PLAYER_OK;
                /* Free text writer. After it, xml buffer can be used */
                xmlFreeTextWriter(iPodCtrlCfg->xmlText);
                iPodCtrlCfg->xmlText = NULL;
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
                rc = IPOD_PLAYER_ERROR;
            }
        }
    }
    
    tagData = (U8 *)iPodCtrlCfg->xmlBuf->content;
    maxTagSize = (U32)iPodCtrlCfg->xmlBuf->use;
    
    /* Check the parameter */
    if((tagData != NULL) && (maxTagSize > 0) && (iPodCtrlCfg->iPodInfo->maxFileSize >= maxTagSize))
    {
        /* Wrote size is more than maximum size */
        if((maxTagSize - iPodCtrlCfg->curTagPos) > iPodCtrlCfg->iPodInfo->maxWriteSize)
        {
            /* Set the size to maximum size */
            writeLen = iPodCtrlCfg->iPodInfo->maxWriteSize;
        }
        /* Wrote size is less than or equal to maximum size */
        else
        {
            /* Set the remaining size */
            writeLen = maxTagSize - iPodCtrlCfg->curTagPos;
        }
        
        /* Write the tag data to iPod */
        rc = iPodWriteiPodFileData(devID, iPodCtrlCfg->curTagPos, tagHandle, (const U8 *)&tagData[iPodCtrlCfg->curTagPos], writeLen);
        if(rc == IPOD_OK)
        {
            /* Add the wrote length to current position */
            iPodCtrlCfg->curTagPos += writeLen;
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
        
        /* Change return code of iPod ctrl to player */
        rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Tag file still remain */
        if(iPodCtrlCfg->curTagPos < maxTagSize)
        {
            /* Set the remain result */
            rc = IPODCORE_TAG_RESULT_DATA_REMAIN;
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;

}

S32 iPodCoreFuncCloseSongTag(U32 devID, IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 tagHandle)
{
    S32 rc = IPOD_ERROR;
    
    /* Parameter check */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Parameter check */
    if(iPodCtrlCfg->xmlBuf == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg->xmlBuf);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    
    /* Close the tag file */
    rc = iPodCloseiPodFile(devID, (U8)tagHandle);
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    /* Free the xml buffer */
    xmlBufferFree(iPodCtrlCfg->xmlBuf);
    iPodCtrlCfg->xmlBuf = NULL;
    iPodCtrlCfg->curTagPos = 0;
    
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


S32 iPodCoreFuncSongTag(U32 devID, IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 handle, IPOD_PLAYER_TAG_TYPE type, const IPOD_PLAYER_TAG_INFO *info)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    if((iPodCtrlCfg == NULL) || (info == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, info);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* For lint */
    devID = devID;
    handle = handle;
    type = type;
    
    
    /* Paramter check */
    if((iPodCtrlCfg->xmlBuf != NULL) && (iPodCtrlCfg->xmlText != NULL))
    {
        /* Add the tag info to XML */
        rc = iPodCoreFuncSetSongTag(iPodCtrlCfg , info);
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->xmlBuf, iPodCtrlCfg->xmlText);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

#if 0
S32 iPodCoreFuncSetOutOptions(U32 option)
{
    S32 rc = IPOD_ERROR;
    U32 optionsBit = 0;
    
    memcpy(&optionsBit, &option, sizeof(optionsBit));
    
    //rc = iPodSetiPodOutOptions(optionsBit);
    rc = IPOD_OK;
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    return rc;
}
#endif 

S32 iPodCoreFuncSetiOSAppInfo(U8 *deviceName, IPOD_PLAYER_IOSAPP_INFO *info, U32 count)
{
    S32 rc = IPOD_ERROR;
    IPOD_IOS_APP *iOSInfo = NULL;
    U32 i = 0;
    
    if((deviceName == NULL) || (info == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, deviceName, info);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(count > 0)
    {
        iOSInfo = calloc(count, sizeof(IPOD_IOS_APP));
        if(iOSInfo != NULL)
        {
            for(i = 0; i < count; i++)
            {
                iOSInfo[i].bundle = info[i].appName;
                iOSInfo[i].protocol = info[i].appURL;
                iOSInfo[i].metaData = 0;
                
                IPOD_LOG_INFO_WRITESTR32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, info[i].appName, i);
                IPOD_LOG_INFO_WRITESTR32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, info[i].appURL, i);
            }
            
            rc = iPodSetConfiOSApp(deviceName, iOSInfo, count);
            if(rc != IPOD_OK)
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
            }
            
            free(iOSInfo);
        }
        else
        {
            rc = IPOD_ERR_NOMEM;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
    }
    else
    {
        /* Clear the iOS information */
        rc = iPodSetConfiOSApp(deviceName, NULL, 0);
        if(rc != IPOD_OK)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncDevInit(U8 *deviceName, IPOD_PLAYER_DEVICE_TYPE devType)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_CONNECTION_TYPE type = IPOD_NO_CONNECT;
    U32 i = 0;
    
    if(deviceName == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, deviceName);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    switch(devType)
    {
        case IPOD_PLAYER_DEVICE_TYPE_USB_DEVICE:
            type = IPOD_USB_HOST_CONNECT;
            rc = IPOD_PLAYER_OK;
            break;
            
        case IPOD_PLAYER_DEVICE_TYPE_USB_HOST:
            type = IPOD_USB_FUNC_CONNECT;
            rc = IPOD_PLAYER_OK;
            break;
            
        case IPOD_PLAYER_DEVICE_TYPE_BT:
            type = IPOD_BT_CONNECT;
            rc = IPOD_PLAYER_OK;
            break;
            
        case IPOD_PLAYER_DEVICE_TYPE_UART:
            type = IPOD_UART_CONNECT;
            rc = IPOD_PLAYER_OK;
            break;
            
        case IPOD_PLAYER_DEVICE_TYPE_UNKNOWN:
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, devType);
            break;
        default :
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, devType);
            break;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = IPOD_PLAYER_ERROR;
        for(i = 0; i < IPODCORE_IPODCTRL_INIT_RETRY_MAX; i++)
        {
            rc = iPodInitDeviceConnection(deviceName, type);
            if(rc >= 0)
            {
                break;
            }
            else
            {
                /* Change return code of iPod ctrl to player */
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
                iPodCoreFuncSleep(IPODCORE_IPODCTRL_INIT_WAIT_TIME);
            }
        }
    }
    
    if(rc < 0)
    {
        rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncDevDeinit(U32 devID)
{
    S32 rc = IPOD_ERROR;
    
    iPodDisconnectDevice(devID);
    rc = IPOD_OK;
    
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncAccSample(U32 devID)
{
    S32 rc = IPOD_ERROR;
    
    /* For lint */
    devID = devID;
    
//    iPodAccAck(devID, IPOD_ACC_ACK_STATUS_SUCCESS);
    rc = IPOD_OK;
    
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
    
}

S32 iPodCoreFuncEndForward(U32 devID)
{
    S32 rc = IPOD_ERROR;
    
    rc = iPodPlayNormal(devID);
    if(rc != IPOD_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncGetCaps(U32 devID, U64 *totalSpace, U32 *maxFileSize, U16 *maxWriteSize)
{
    S32 rc = IPOD_ERROR;
    IPOD_STORAGE_CAPS storage;
    
    memset(&storage, 0, sizeof(storage));
    rc = iPodGetiPodCaps(devID, &storage);
    if(rc == IPOD_OK)
    {
        *totalSpace = storage.totalSpace;
        *maxFileSize = storage.maxFileSize;
        *maxWriteSize = storage.maxWriteSize - IPOD_PLAYER_PROTOCOL_HEADER_LEN;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


S32 iPodCoreFuncIntGetStatus(U32 devID, U8 *status, U32 *pos)
{
    S32 rc = IPOD_ERROR;
    IPOD_PLAYER_STATE state;
    U32 length = 0;
    U32 position = 0;
    
    if(status == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, status);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    rc = iPodGetPlayStatus(devID, &state, &length, &position);
    if(rc == IPOD_OK)
    {
        *pos = position;
        switch(state)
        {
            case IPOD_PLAYER_STATE_STOPPED:
                *status = IPOD_PLAYER_PLAY_STATUS_STOP;
                break;
                
            case IPOD_PLAYER_STATE_PLAYING:
                *status = IPOD_PLAYER_PLAY_STATUS_PLAY;
                break;
                
            case IPOD_PLAYER_STATE_PAUSED:
                *status = IPOD_PLAYER_PLAY_STATUS_PAUSE;
                break;
                
            default:
                rc = IPOD_ERROR;
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, state);
                break;
        }
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncIntGetVideoSetting(U32 devID, IPOD_PLAYER_VIDEO_SETTING *setting)
{
    S32 rc = IPOD_ERROR;
    IPOD_PREFERENCE_CLASS_ID classId = IPOD_VOUT_SETTING;
    IPOD_PREFERENCE_SETTING_ID settingId;
    
    if(setting == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, setting);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get Video Output Setting */
    classId = IPOD_VOUT_SETTING;
    rc = iPodGetiPodPreferences(devID, classId, &settingId);
    if(rc == IPOD_OK)
    {
        switch(settingId.videoOutSetting)
        {
            case IPOD_VOUT_SETTING_OFF:
                setting->videoOutput = 0;
                break;
                
            case IPOD_VOUT_SETTING_ON:
                setting->videoOutput = 1;
                break;
                
            default:
                setting->videoOutput = IPODCORE_UNKNOWN_VALUE;
                break;
        }
    }
    else
    {
        setting->videoOutput = IPODCORE_UNKNOWN_VALUE;
        IPOD_DLT_WARN("iPodGetiPodPreferences :rc=%d", rc);
    }
    
    /* Get Screen Setting */
    classId = IPOD_VSCREEN_CFG;
    rc = iPodGetiPodPreferences(devID, classId, &settingId);
    if(rc == IPOD_OK)
    {
        switch(settingId.screenCfg)
        {
            case IPOD_VSCREEN_CFG_FILL:
                setting->screenSetting = 0;
                break;
                
            case IPOD_VSCREEN_CFG_FIT:
                setting->screenSetting = 1;
                break;
                
            default:
                setting->screenSetting = IPODCORE_UNKNOWN_VALUE;
                break;
        }
    }
    else
    {
        setting->screenSetting = IPODCORE_UNKNOWN_VALUE;
        IPOD_DLT_WARN("iPodGetiPodPreferences :rc=%d", rc);
    }
    
    /* Get Video Signal Format */
    classId = IPOD_VSIG_FORMAT;
    rc = iPodGetiPodPreferences(devID, classId, &settingId);
    if(rc == IPOD_OK)
    {
        switch(settingId.signalFormat)
        {
            case IPOD_VSIG_FORMAT_NTSC:
                setting->videoSignal = 0;
                break;
                
            case IPOD_VSIG_FORMAT_PAL:
                setting->videoSignal = 1;
                break;
                
            default:
                setting->videoSignal = IPODCORE_UNKNOWN_VALUE;
                break;
        }
    }
    else
    {
        setting->videoSignal = IPODCORE_UNKNOWN_VALUE;
        IPOD_DLT_WARN("iPodGetiPodPreferences :rc=%d", rc);
    }
    
    /* Get Line-out useage */
    classId = IPOD_VLINE_OUT_USAGE;
    rc = iPodGetiPodPreferences(devID, classId, &settingId);
    if(rc == IPOD_OK)
    {
        switch(settingId.lineOut)
        {
            case IPOD_VLINE_OUT_USAGE_NOT_USED:
                setting->videoLineout = 0;
                break;
                
            case IPOD_VLINE_OUT_USAGE_USED:
                setting->videoLineout = 1;
                break;
                
            default:
                setting->videoLineout = IPODCORE_UNKNOWN_VALUE;
                break;
        }
    }
    else
    {
        setting->videoLineout = IPODCORE_UNKNOWN_VALUE;
        IPOD_DLT_WARN("iPodGetiPodPreferences :rc=%d", rc);
    }
    
    /* Get Video-Out Connection */
    classId = IPOD_VOUT_CONNECT;
    rc = iPodGetiPodPreferences(devID, classId, &settingId);
    if(rc == IPOD_OK)
    {
        switch(settingId.videoOutConnection)
        {
            case IPOD_VOUT_CONNECT_NONE:
                setting->videoCable = 0;
                break;
                
            case IPOD_VOUT_CONNECT_COMPOSIT:
                setting->videoCable = 1;
                break;
                
            case IPOD_VOUT_CONNECT_S_VIDEO:
                setting->videoCable = 2;
                break;
                
            case IPOD_VOUT_CONNECT_COMPONENT:
                setting->videoCable = 3;
                break;
                
            default:
                setting->videoCable = IPODCORE_UNKNOWN_VALUE;
                break;
        }
    }
    else
    {
        setting->videoCable = IPODCORE_UNKNOWN_VALUE;
        IPOD_DLT_WARN("iPodGetiPodPreferences :rc=%d", rc);
    }
    
    /* Get Closed captioning */
    classId = IPOD_VCLOSED_CAP;
    rc = iPodGetiPodPreferences(devID, classId, &settingId);
    if(rc == IPOD_OK)
    {
        switch(settingId.closedCaptioning)
        {
            case IPOD_VCLOSED_CAP_OFF:
                setting->videoCaption = 0;
                break;
                
            case IPOD_VCLOSED_CAP_ON:
                setting->videoCaption = 1;
                break;
                
            default:
                setting->videoCaption = IPODCORE_UNKNOWN_VALUE;
                break;
        }
    }
    else
    {
        setting->videoCaption = IPODCORE_UNKNOWN_VALUE;
        IPOD_DLT_WARN("iPodGetiPodPreferences :rc=%d", rc);
    }
    
    /* Get Video Monitor Aspect Ratio  */
    classId = IPOD_VASP_RATIO;
    rc = iPodGetiPodPreferences(devID, classId, &settingId);
    if(rc == IPOD_OK)
    {
        switch(settingId.aspectRatio)
        {
            case IPOD_VASP_RATIO_FULL:
                setting->aspectRatio = 0;
                break;
                
            case IPOD_VASP_RATIO_WIDE:
                setting->aspectRatio = 1;
                break;
                
            default:
                setting->aspectRatio = IPODCORE_UNKNOWN_VALUE;
                break;
        }
    }
    else
    {
        setting->aspectRatio = IPODCORE_UNKNOWN_VALUE;
        IPOD_DLT_WARN("iPodGetiPodPreferences :rc=%d", rc);
    }
    
    /* Get Subtitles */
    classId = IPOD_VSUBTITLES;
    rc = iPodGetiPodPreferences(devID, classId, &settingId);
    if(rc == IPOD_OK)
    {
        switch(settingId.subTitles)
        {
            case IPOD_VSUBTITLES_OFF:
                setting->subtitle = 0;
                break;
                
            case IPOD_VSUBTITLES_ON:
                setting->subtitle = 1;
                break;
                
            default:
                setting->subtitle = IPODCORE_UNKNOWN_VALUE;
                break;
        }
    }
    else
    {
        setting->subtitle = IPODCORE_UNKNOWN_VALUE;
        IPOD_DLT_WARN("iPodGetiPodPreferences :rc=%d", rc);
    }
    
    /* Get Alternate Audio Channel */
    classId = IPOD_VALT_AUD_CHANNEL;
    rc = iPodGetiPodPreferences(devID, classId, &settingId);
    if(rc == IPOD_OK)
    {
        switch(settingId.audioChannel)
        {
            case IPOD_VALT_AUD_CHANNEL_OFF:
                setting->altanateAudio = 0;
                break;
                
            case IPOD_VALT_AUD_CHANNEL_ON:
                setting->altanateAudio = 1;
                break;
                
            default:
                setting->altanateAudio = IPODCORE_UNKNOWN_VALUE;
                break;
        }
    }
    else
    {
        setting->altanateAudio = IPODCORE_UNKNOWN_VALUE;
        IPOD_DLT_WARN("iPodGetiPodPreferences :rc=%d", rc);
    }
    
    /* Get Power */
    /*
    classId = IPOD_VALT_AUD_CHANNEL;
    rc = iPodGetiPodPreferences(devID, classId, &settingId);
    if(rc == IPOD_OK)
    {
        switch(settingId)
        {
            case IPOD_VALT_AUD_CHANNEL_OFF:
                setting->altanateAudio = 0;
                break;
                
            case IPOD_VALT_AUD_CHANNEL_ON:
                setting->altanateAudio = 1;
                break;
                
            default:
                setting->altanateAudio = IPODCORE_UNKNOWN_VALUE;
                break;
        }
    }
    else
    {
        setting->altanateAudio = IPODCORE_UNKNOWN_VALUE;
    }
    */
    /* Get Voice Overl */
    /*
    classId = IPOD_VALT_AUD_CHANNEL;
    rc = iPodGetiPodPreferences(devID, classId, &settingId);
    if(rc == IPOD_OK)
    {
        switch(settingId)
        {
            case IPOD_VALT_AUD_CHANNEL_OFF:
                setting->altanateAudio = 0;
                break;
                
            case IPOD_VALT_AUD_CHANNEL_ON:
                setting->altanateAudio = 1;
                break;
                
            default:
                setting->altanateAudio = IPODCORE_UNKNOWN_VALUE;
                break;
        }
    }
    else
    {
        setting->altanateAudio = IPODCORE_UNKNOWN_VALUE;
    }
    
    */
    
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncIntGetCurrentTrackIndex(U32 devID, U32 *trackIndex)
{
    S32 rc = IPOD_ERROR;
    
    if(trackIndex == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, trackIndex);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    rc = iPodGetCurrentPlayingTrackIndex(devID);
    if(rc >= 0)
    {
        *trackIndex = rc;
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        IPOD_DLT_WARN("iPodGetCurrentPlayingTrackIndex :rc=%d", rc);
    }
    
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncIntGetCurrentChapterIndex(U32 devID, U32 *chapterIndex, U32 *chapterTotal)
{
    S32 rc = IPOD_ERROR;
    S32 curChap = 0;
    S32 totalChap = 0;
    
    if((chapterIndex == NULL) || (chapterTotal == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, chapterIndex, chapterTotal);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    rc = iPodGetCurrentPlayingTrackChapterInfo(devID, &curChap, &totalChap);
    if(rc == IPOD_OK)
    {
        if(totalChap > 0)
        {
            *chapterIndex = curChap;
            *chapterTotal = totalChap;
        }
        else
        {
            IPOD_DLT_WARN("iPodGetCurrentPlayingTrackChapterInfo :rc=%d, totalChap=%d", rc, totalChap);
            rc = IPOD_ERROR;
        }
    }
    else
    {
        IPOD_DLT_WARN("iPodGetCurrentPlayingTrackChapterInfo :rc=%d", rc);
    }
    
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreFuncIntGetCurrentChapterStatus(U32 devID, U32 chapterIndex, U32 *position)
{
    S32 rc = IPOD_ERROR;
    U32 totalLen = 0;
    U32 curLen = 0;
    
    rc = iPodGetCurrentPlayingTrackChapterPlayStatus(devID, chapterIndex, &totalLen, &curLen);
    if(rc == IPOD_OK)
    {
        *position = curLen;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


S32 iPodCoreFuncGetiPodOutOptions(U32 iPodID, U8 types, U32 *optionsBits)
{
   S32 rc = IPOD_ERROR;
   U32 optionsBitsval = 0;
   U8 typesval = 0;
    
    switch(types)
    {
    case IPOD_PLAYER_HMI_FEATURE_TYPE_CURRENT:
        typesval = 0x01;
        rc = IPOD_OK;
        break;
        
    case IPOD_PLAYER_HMI_FEATURE_TYPE_ALL:
        typesval = 0x00;
        rc = IPOD_OK;
        break;
        
    default:
        rc = IPOD_BAD_PARAMETER;
        break;
    }
    
    if(rc == IPOD_OK)
    {
        rc = iPodGetiPodOutOptions(iPodID,typesval,&optionsBitsval);
        
        if(rc == IPOD_OK)
        {
            if(optionsBitsval & IPODCORE_MASK_BIT_0)
            {
                *optionsBits |= IPOD_PLAYER_HMI_FEATURE_MASK_AUDIO_CONTENTS ;
            }
            if(optionsBitsval & IPODCORE_MASK_BIT_1)
            {
                *optionsBits |= IPOD_PLAYER_HMI_FEATURE_MASK_PHONE_CALL;
            }
            if(optionsBitsval & IPODCORE_MASK_BIT_2)
            {
                *optionsBits |= IPOD_PLAYER_HMI_FEATURE_MASK_SMS_MMS;
            }
            if(optionsBitsval & IPODCORE_MASK_BIT_3)
            {
                /* do nothing */
            }
            if(optionsBitsval & IPODCORE_MASK_BIT_4)
            {
                *optionsBits |= IPOD_PLAYER_HMI_FEATURE_MASK_VOICEMAIL;
            }
            if(optionsBitsval & IPODCORE_MASK_BIT_5)
            {
                *optionsBits |= IPOD_PLAYER_HMI_FEATURE_MASK_PUSH_NOTIFY;
            }
            if(optionsBitsval & IPODCORE_MASK_BIT_6)
            {
                *optionsBits |= IPOD_PLAYER_HMI_FEATURE_MASK_ALARM;
            }
            if(optionsBitsval & IPODCORE_MASK_BIT_7)
            {
                /* do nothing */
            }
            if(optionsBitsval & IPODCORE_MASK_BIT_8)
            {
                *optionsBits |= IPOD_PLAYER_HMI_FEATURE_MASK_TEST_PATTERN;
            }
            if(optionsBitsval & IPODCORE_MASK_BIT_9)
            {
                *optionsBits |= IPOD_PLAYER_HMI_FEATURE_MASK_MINIMAM_UI;
            }
            if(optionsBitsval & IPODCORE_MASK_BIT_10)
            {
                *optionsBits |= IPOD_PLAYER_HMI_FEATURE_MASK_FULL_UI;
            }
        }
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    return rc;
}

S32 iPodCoreFuncSetiPodOutOptions(U32 iPodID, U32 optionsBits)
{
    S32 rc = IPOD_ERROR;
    U32 optionBitsval = 0;
    
    if(optionsBits & IPOD_PLAYER_HMI_FEATURE_MASK_AUDIO_CONTENTS)
    {
        optionBitsval |= IPODCORE_MASK_BIT_0;
    }
    if(optionsBits & IPOD_PLAYER_HMI_FEATURE_MASK_PHONE_CALL)
    {
        optionBitsval |= IPODCORE_MASK_BIT_1;
    }
    if(optionsBits & IPOD_PLAYER_HMI_FEATURE_MASK_SMS_MMS)
    {
        optionBitsval |= IPODCORE_MASK_BIT_2;
    }
    if(optionsBits & IPOD_PLAYER_HMI_FEATURE_MASK_VOICEMAIL)
    {
        optionBitsval |= IPODCORE_MASK_BIT_4;
    }
    if(optionsBits & IPOD_PLAYER_HMI_FEATURE_MASK_PUSH_NOTIFY)
    {
        optionBitsval |= IPODCORE_MASK_BIT_5;
    }
    if(optionsBits & IPOD_PLAYER_HMI_FEATURE_MASK_ALARM)
    {
        optionBitsval |= IPODCORE_MASK_BIT_6;
    }
    if(optionsBits & IPOD_PLAYER_HMI_FEATURE_MASK_TEST_PATTERN)
    {
        optionBitsval |= IPODCORE_MASK_BIT_8;
    }
    if(optionsBits & IPOD_PLAYER_HMI_FEATURE_MASK_MINIMAM_UI)
    {
        optionBitsval |= IPODCORE_MASK_BIT_9;
    }
    if(optionsBits & IPOD_PLAYER_HMI_FEATURE_MASK_FULL_UI)
    {
        optionBitsval |= IPODCORE_MASK_BIT_10;
    }
    
    rc = iPodSetiPodOutOptions(iPodID, optionBitsval);
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    return rc;
}

S32 iPodCoreFuncDevStateChangeEvent(U32 iPodID, U8 status)
{
    S32 rc = IPOD_ERROR;
    U8 statusval = 0;
    
    switch(status)
    {
    case IPOD_PLAYER_HMI_APP_STATUS_HIDE_HMI_DISPLAY:
        statusval = 0x00;
        rc = IPOD_OK;
        break;
    case IPOD_PLAYER_HMI_APP_STATUS_SHOW_HMI_DISPLAY:
        statusval = 0x01;
        rc = IPOD_OK;
        break;
    case IPOD_PLAYER_HMI_APP_STATUS_HIDE_HMI_AUDIO:
        statusval = 0x02;
        rc = IPOD_OK;
        break;
    case IPOD_PLAYER_HMI_APP_STATUS_SHOW_HMI_AUDIO:
        statusval = 0x03;
        rc = IPOD_OK;
        break;
    case IPOD_PLAYER_HMI_APP_STATUS_SHOW_BRIGHT:
        statusval = 0x04;
        rc = IPOD_OK;
        break;
    case IPOD_PLAYER_HMI_APP_STATUS_SHOW_DIMMED: 
        statusval = 0x05;
        rc = IPOD_OK;
        break;
    default:
        rc = IPOD_BAD_PARAMETER;
        break;
    }
    
    if(rc == IPOD_OK)
    {
        rc = iPodDevStateChangeEvent(iPodID, statusval);
    }
    
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    return rc;
}

S32 iPodCoreFuncOutButtonStatus(U32 iPodID, U8 source, U32 statusBits)
{
    S32 rc = IPOD_ERROR;
    U8 tempSource = 0;
    U32 tempStatusBits = 0;
    
    if(source == IPOD_PLAYER_HMI_BUTTON_SOURCE_CONSOLE)
    {
        tempSource = 0x00;
        rc = IPOD_OK;
    }
    else if(source == IPOD_PLAYER_HMI_BUTTON_SOURCE_WHEEL)
    {
        tempSource = 0x01;
        rc = IPOD_OK;
    }
    else if(source == IPOD_PLAYER_HMI_BUTTON_SOURCE_DASHBOARD)
    {
        tempSource = 0x02;
        rc = IPOD_OK;
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
    }
    
    if(rc == IPOD_OK)
    {
        if(statusBits & IPOD_PLAYER_HMI_BUTTON_EVENT_UP)
        {
            tempStatusBits |= 1 << 3;
        }
        
        if(statusBits & IPOD_PLAYER_HMI_BUTTON_EVENT_DOWN)
        {
            tempStatusBits |= 1 << 4;
        }
        if(statusBits & IPOD_PLAYER_HMI_BUTTON_EVENT_RIGHT)
        {
            tempStatusBits |= 1 << 2;
        }
        if(statusBits & IPOD_PLAYER_HMI_BUTTON_EVENT_LEFT)
        {
            tempStatusBits |= 1 << 1;
        }
        if(statusBits & IPOD_PLAYER_HMI_BUTTON_EVENT_SELECT)
        {
            tempStatusBits |= 1 << 0;
        }
        if(statusBits & IPOD_PLAYER_HMI_BUTTON_EVENT_MENU)
        {
            tempStatusBits |= 1 << 5;
        }
        if(statusBits == IPOD_PLAYER_HMI_BUTTON_EVENT_RELEASE)
        {
            tempStatusBits = 0;
        }
        
        rc = iPodOutButtonStatus(iPodID, tempSource, tempStatusBits);
    }
    
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    return rc;
    
}

S32 iPodCoreFuncRotationInputStatus(U32 iPodID, U32 durationMs, IPOD_PLAYER_HMI_ROTATION_INFO rotation, U16 move)
{
    S32 rc = IPOD_ERROR;
    IPOD_ROTATION_INFO rotationval = {0,0,0,0,0,0,0,0};
    
    rotationval.durationMs = durationMs;
    rotationval.source = rotation.source;
    rotationval.controllerType = rotation.device;
    
    if(rotationval.direction == IPOD_PLAYER_HMI_ROTATION_DIRECTION_LEFT)
    {
        rotationval.direction = 0x00;
        rc = IPOD_OK;
    }
    else if(rotationval.direction == IPOD_PLAYER_HMI_ROTATION_DIRECTION_RIGHT)
    {
        rotationval.direction = 0x01;
        rc = IPOD_OK;
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
    }
    if(rc == IPOD_OK)
    {
        rotationval.action = rotation.action;
        
        rotationval.type = rotation.type;
        rotationval.moved = move;
        rotationval.total = rotation.max;
        
        rc = iPodRotationInputStatus(iPodID, rotationval);
    }
    /* Change return code of iPod ctrl to player */
    rc = iPodCoreFuncChangeRetCodeToPlayer(rc);
    
    return rc;
}

