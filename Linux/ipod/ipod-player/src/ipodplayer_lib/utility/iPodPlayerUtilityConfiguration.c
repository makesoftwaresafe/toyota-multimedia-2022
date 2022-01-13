#include <errno.h>
#include "iPodPlayerUtilityConfiguration.h"


/* xml paser information */
struct IPOD_UTIL_CFG_INFO
{
    xmlDocPtr document;                             /* for xml document         */
    xmlXPathContextPtr xpath;                       /* for xml xpath context    */
    xmlXPathObjectPtr object;                       /* for xml object           */
};

/* local function prototype */
static xmlNodePtr iPodUtilGetCfNodeByKeyName(xmlDocPtr doc, xmlXPathObjectPtr object, const U8 *key);
static S32 iPodUtilGetCfn(xmlDocPtr doc, xmlNodePtr node, S32 *value);
static S32 iPodUtilGetCfs(xmlDocPtr doc, xmlNodePtr node, IPOD_UTIL_CFG_STR *value);


/* get the xmlNode by a key */
static xmlNodePtr iPodUtilGetCfNodeByKeyName(xmlDocPtr doc, xmlXPathObjectPtr object, const U8 *key)
{
    U32 i = 0;                                      /* for loop           */
    U32 nodeMax = 0;                                /* for number of node */
    xmlNodeSetPtr nodeset = NULL;                   /* for node list      */
    xmlNodePtr node = NULL;                         /* for current node   */
    xmlChar *element = NULL;                        /* for node content   */
    
    /* check parameter */
    if((doc == NULL) || (object == NULL) || (key == NULL))
    {
        /* invalid parameter */
        return NULL;
    }
    
    /* get number of nodlist and xmlNodeSet */
    nodeset = object->nodesetval;
    nodeMax = (U32)object->nodesetval->nodeNr;
    /* search node list */
    for(i = 0; i < nodeMax; i++)
    {
        /* get the first closest next sibling of the node */
        node = xmlNextElementSibling(nodeset->nodeTab[i]->xmlChildrenNode);
        if(node != NULL)
        {
            /* get content of node */
            element = xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            if(element != NULL)
            {
                /* match with the required key */
                if(strncmp((const char *)key, (const char *)element, IPOD_UTIL_CFG_STR_MAX_SIZE) == 0)
                {
                    /* get first closest next sibling */
                    node = xmlNextElementSibling(node);
                    /* free element */
                    xmlFree(element);
                    break;
                }
                else
                {
                    /* free element */
                    xmlFree(element);
                }
            }
        }
    }
    
    return node;
}


/* get number value of node */
static S32 iPodUtilGetCfn(xmlDocPtr doc, xmlNodePtr node, S32 *value)
{
    S32 rc = IPOD_UTIL_ERROR;                       /* for resault        */
    S32 int_param = 0;                              /* for integer value  */
    U8  *err = NULL;                                /* for error char     */
    xmlChar *element = NULL;                        /* for node content   */
    xmlAttrPtr attr = NULL;                         /* for node attribute */
    
    
    /* check parameter */
    if((doc == NULL) || (node == NULL) || (value == NULL))
    {
        /* invalid parameter */
        return IPOD_UTIL_ERR_INVALID_PARAMETER;
    }
    
    /* check parameter */
    if((node->properties == NULL) || (node->xmlChildrenNode == NULL))
    {
        /* invalid parameter */
        return IPOD_UTIL_ERR_INVALID_PARAMETER;
    }
    
    /* get attribute */
    attr = node->properties;
    /* check value type */
    rc = strncmp((const char *)attr->children->content, IPOD_UTIL_CFG_INT_TAG, IPOD_UTIL_CFG_STR_MAX_SIZE);
    if(rc == 0)
    {
        /* get content of node */
        element = xmlNodeListGetString(doc, node->xmlChildrenNode, 0);
        if(element != NULL)
        {
            errno = 0;
            /* conver string to int value */
            int_param = (S32)strtoul((char *)element, (char **)&err, 0);
            if((errno == 0) && ((err != NULL) && (*err == '\0')))
            {
                /* set value */
                *value = int_param;
                rc = IPOD_UTIL_OK;
            }
            else
            {
                /* failed to convert */
                rc = IPOD_UTIL_ERROR;
            }
            /* free element */
            xmlFree(element);
        }
        else
        {
            /* failed to get element */
            rc = IPOD_UTIL_ERROR;
        }
    }
    else
    {
        /* vale type not match */
        rc = IPOD_UTIL_ERROR;
    }
    
    return rc;
}


/* get string value of node */
static S32 iPodUtilGetCfs(xmlDocPtr doc, xmlNodePtr node, IPOD_UTIL_CFG_STR *value)
{
    S32 rc = IPOD_UTIL_ERROR;                       /* for resault        */
    U32 size = 0;                                   /* for size of string */
    xmlChar *element = NULL;                        /* for node content   */
    xmlAttrPtr attr = NULL;                         /* for node attribute */
    
    
    /* check parameter */
    if((doc == NULL) || (node == NULL) || (value == NULL))
    {
        /* invalid parameter */
        return IPOD_UTIL_ERR_INVALID_PARAMETER;
    }
    
    /* check parameter */
    if(node->properties == NULL)
    {
        /* invalid parameter */
        return IPOD_UTIL_ERR_INVALID_PARAMETER;
    }
    
    if(node->xmlChildrenNode != NULL)
    {
        /* get attribute */
        attr = node->properties;
        /* check value type */
        rc = strncmp((const char *)attr->children->content, IPOD_UTIL_CFG_STR_TAG, IPOD_UTIL_CFG_STR_MAX_SIZE);
        if(rc == 0)
        {
            /* get content of node */
            element = xmlNodeListGetString(doc, node->xmlChildrenNode, 0);
            if(element != NULL)
            {
                /* get string length */
                size = strnlen((char *)element, IPOD_UTIL_CFG_STR_MAX_SIZE - 1);
                if(size < value->size)
                {
                    /* set size of string */
                    value->size = size;
                }
                else
                {
                    /* leave the last byte for adding '\0' */
                    value->size -= 1;
                }
                /* copy string value */
                strncpy((char *)value->strVal, (const char *)element, value->size);
                value->strVal[value->size] = '\0';
                /* free element */
                xmlFree(element);
                rc = IPOD_UTIL_OK;
            }
            else
            {
                /* failed to get element */
                rc = IPOD_UTIL_ERROR;
            }
        }
        else
        {
            /* vale type not match */
            rc = IPOD_UTIL_ERROR;
        }
    }
    else
    {
        /* set value */
        value->strVal[0] = '\0';
        value->size = 0;
        rc = IPOD_UTIL_OK;
    }
    
    return rc;
}


/* get number array value of node */
S32 iPodUtilGetNumCfn(IPOD_UTIL_CFG_HANDLE cfgHandle, const U8 *key, U32 maxNum, S32 *value)
{
    S32 rc = IPOD_UTIL_ERROR;                       /* for resault        */
    U32 i = 0;                                      /* for loop           */
    S32 int_param = 0;                              /* for integer value  */
    xmlDocPtr doc = NULL;                           /* for xml document   */
    xmlXPathObjectPtr object = NULL;                /* for xml object     */
    xmlNodePtr node = NULL;                         /* for current node   */
    
    /* check parameter */
    if((cfgHandle == NULL) || (key == NULL) || (value == NULL))
    {
        /* invalid parameter */
        return IPOD_UTIL_ERR_INVALID_PARAMETER;
    }
    
    /* check parameter */
    if((cfgHandle->document == NULL) || (cfgHandle->object == NULL))
    {
        /* invalid parameter */
        return IPOD_UTIL_ERR_INVALID_PARAMETER;
    }
    
    /* get global parameter */
    doc = cfgHandle->document;
    object = cfgHandle->object;
    
    /* get node by key name */
    node = iPodUtilGetCfNodeByKeyName(doc, object, key);
    for(i = 0; (i < maxNum) && (node != NULL); i++)
    {
        /* get number from node */
        rc = iPodUtilGetCfn(doc, node, &int_param);
        if(rc != IPOD_UTIL_OK)
        {
            /* failed to get value */
            break;
        }
        /* set value */
        value[i] = int_param;
        /* shift to next node */
        node = xmlNextElementSibling(node);
    }
    
    if(rc == IPOD_UTIL_OK)
    {
        rc = i;
    }
    
    return rc;
}


/* get string array value of node */
S32 iPodUtilGetNumCfs(IPOD_UTIL_CFG_HANDLE cfgHandle, const U8 *key, U32 maxNum, IPOD_UTIL_CFG_STR *value)
{
    S32 rc = IPOD_UTIL_ERROR;                       /* for resault        */
    U32 i = 0;                                      /* for loop           */
    xmlDocPtr doc = NULL;                           /* for xml document   */
    xmlXPathObjectPtr object = NULL;                /* for xml object     */
    xmlNodePtr node = NULL;                         /* for next node      */
    
    
    /* check parameter */
    if((cfgHandle == NULL) || (key == NULL) || (value == NULL))
    {
        /* invalid parameter */
        return IPOD_UTIL_ERR_INVALID_PARAMETER;
    }
    
    /* check parameter */
    if((cfgHandle->document == NULL) || (cfgHandle->object == NULL))
    {
        /* invalid parameter */
        return IPOD_UTIL_ERR_INVALID_PARAMETER;
    }
    
    /* get global parameter */
    doc = cfgHandle->document;
    object = cfgHandle->object;
    
    /* get node by key name */
    node = iPodUtilGetCfNodeByKeyName(doc, object, key);
    for(i = 0; (i < maxNum) && (node != NULL); i++)
    {
        /* get number from node */
        rc = iPodUtilGetCfs(doc, node, &value[i]);
        if(rc != IPOD_UTIL_OK)
        {
            break;
        }
        /* shift to next node */
        node = xmlNextElementSibling(node);
    }
    
    if(rc == IPOD_UTIL_OK)
    {
        rc = i;
    }
    return rc;
}


/* initialize util cfg function */
IPOD_UTIL_CFG_HANDLE iPodUtilInitCf(const U8 *xmlFile, const U8* rootTag)
{
    xmlDocPtr doc = NULL;                               /* for xml document     */
    xmlXPathContextPtr xpath = NULL;                    /* for xml xpath contxt */
    xmlXPathObjectPtr object = NULL;                    /* for xml object       */
    xmlChar tag_str[IPOD_UTIL_CFG_STR_MAX_SIZE] = {0};  /* for root tag string  */
    xmlChar cfg_path[IPOD_UTIL_CFG_PATH_MAX_LEN] = {0}; /* for cfg file path    */
    struct IPOD_UTIL_CFG_INFO *cfg_handle = NULL;       /* for cfg handle       */
    
    /* check parameter */
    if((xmlFile == NULL) || (rootTag == NULL))
    {
        /* invalid parameter */
        return NULL;
    }
    
    cfg_handle = malloc(sizeof(struct IPOD_UTIL_CFG_INFO));
    if(cfg_handle != NULL)
    {
        snprintf((char *)cfg_path, sizeof(cfg_path), "%s%s%s", IPOD_UTIL_CFG_FILE_DIR_NAME, (const char *)xmlFile, IPOD_UTIL_CFG_FILE_EXTENSION);
        /* parse an XML file and build a tree */
        doc = xmlParseFile((const char *)cfg_path);
        if(doc == NULL)
        {
            /* free handle */
            free(cfg_handle);
            cfg_handle = NULL;
        }
    }
    if(doc != NULL)
    {
        /* create a new xmlXPathContext */
        xpath = xmlXPathNewContext(doc);
        if(xpath == NULL)
        {
            /* free xmlDocument and handle*/
            xmlFreeDoc(doc);
            free(cfg_handle);
            cfg_handle = NULL;
        }
    }
    /* get xmlXPathContext ok */
    if(xpath != NULL)
    {
        /* get XPath expression */
        snprintf((char *)tag_str, sizeof(tag_str), "%s%s", (char *)rootTag, IPOD_UTIL_CFG_ITEM_TAG);
        /* evaluate the XPath expression */
        object = xmlXPathEvalExpression(tag_str, xpath);
        if((object != NULL) && (cfg_handle != NULL))
        {
            /* set handle parameter */
            cfg_handle->document = doc;
            cfg_handle->xpath = xpath;
            cfg_handle->object = object;
        }
        else
        {
            /* free xmlXPathContext and doc */
            xmlXPathFreeContext(xpath);
            xmlFreeDoc(doc);
            free(cfg_handle);
            cfg_handle = NULL;
        }
    }
    
    return cfg_handle;
}


/* finalize util cfg function */
void iPodUtilDeInitCf(IPOD_UTIL_CFG_HANDLE cfgHandle)
{
    /* check parameter */
    if(cfgHandle == NULL)
    {
        /* invalid parameter */
        return;
    }
    
    if(cfgHandle->object != NULL)
    {
        /* free object */
        xmlXPathFreeObject(cfgHandle->object);
    }
    
    if(cfgHandle->xpath != NULL)
    {
        /* free xmlXPathContext */
        xmlXPathFreeContext(cfgHandle->xpath);
    }
    
    if(cfgHandle->document != NULL)
    {
        /* free xmlDocument */
        xmlFreeDoc(cfgHandle->document);
    }
    
    free(cfgHandle);
    cfgHandle = NULL;
    
    return;
}

