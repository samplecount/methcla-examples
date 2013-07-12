#include <assert.h>
#include <stddef.h>
#include <string.h>

typedef enum
{
    kMescalineString
  , kMescalineInt
  , kMescalineFloat
} MescalineValueType;

struct MescalineValue
{
    MescalineValueType type;
    union {
        const char* stringValue;
        int         intValue;
        float       floatValue;
    } data;
};
typedef struct MescalineValue MescalineValue;

struct MescalineAssoc
{
    struct MescalineAssoc*  next;
    const char*             key;
    MescalineValue          value;
};
typedef struct MescalineAssoc MescalineAssoc;

struct MescalineMetaData
{
    MescalineAssoc* data;
};

static inline void MescalineAssocInitString(MescalineAssoc* self, const char* key, const char* value)
{
    self->next = NULL;
    self->key = key;
    self->value.type = kMescalineString;
    self->value.data.stringValue = value;
}

static inline void MescalineAssocInitInt(MescalineAssoc* self, const char* key, int value)
{
    self->next = NULL;
    self->key = key;
    self->value.type = kMescalineInt;
    self->value.data.intValue = value;
}

static inline void MescalineAssocInitFloat(MescalineAssoc* self, const char* key, float value)
{
    self->next = NULL;
    self->key = key;
    self->value.type = kMescalineFloat;
    self->value.data.floatValue = value;
}

static inline const char* MescalineAssocGetString(MescalineAssoc* self, const char* defaultValue)
{
    return self->value.type == kMescalineString ? self->value.data.stringValue : defaultValue;
}

static inline int MescalineAssocGetInt(MescalineAssoc* self)
{
    switch (self->value.type) {
        case kMescalineString:
            return atoi(self->value.data.stringValue);
        case kMescalineInt:
            return self->value.data.intValue;
        case kMescalineFloat:
            return (int)self->value.data.floatValue;
    }
    return 0;
}

static inline int MescalineAssocGetFloat(MescalineAssoc* self)
{
    switch (self->value.type) {
        case kMescalineString:
            return atof(self->value.data.stringValue);
        case kMescalineInt:
            return (float)self->value.data.intValue;
        case kMescalineFloat:
            return self->value.data.floatValue;
    }
    return 0;
}

static inline void MescalineMetaDataInit(MescalineMetaData* self)
{
    self->data = NULL;
}

static inline void MescalineMetaDataInsert(MescalineMetaData* self, MescalineAssoc* assoc)
{
    assoc->next = self->data;
    self->data = assoc;
}

enum MethclaUINodeType
{
    kMethclaUIContainer
  , kMethclaUIControl
};

struct MethclaUINode
{
    MethclaUINodeType type;
    MethclaUINode*    next;
};
typedef struct MethclaUINode MethclaUINode;

struct MethclaUIControl
{
    MethclaUINode             node;
    const MethclaControlSpec* spec;
    const char*                 type;
    const char*                 label;
};
typedef struct MethclaUIElement MethclaUIElement;

static inline void MethclaUIControlInit(MethclaUIControl* self, const MethclaControlSpec* spec, const char* type, const char* label)
{
    self->node.type = kMethclaUIControl;
    self->node.next = NULL;
    self->spec = spec;
    self->type = type;
    self->label = label;
}

struct MethclaUIContainer
{
    MethclaUINode     node;
    const char*         type;
    const char*         label;
    MethclaUINode*    children;
};
typedef struct MethclaUIContainer MethclaUIContainer;

static inline void MethclaUIContainerInit(MethclaUIContainer* self, const char* type, const char* label)
{
    self->node.type = kMethclaUIContainer;
    self->node.next = NULL;
    self->type = type;
    self->label = label;
    self->children = NULL;
}

