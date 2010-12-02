#define SIPSIMPLE_PROTOCOL_NAME LPGEN("SIP/SIMPLE")
#define SIP_UNIQUEID "sip_screenname"

#define snprintf sprintf_s

typedef struct sipe_backend_private
{
	PROTO_INTERFACE proto;
	struct sipe_core_public *sip;
	HANDLE m_hServerNetlibUser;
} SIPPROTO;

struct miranda_sipe_ack_args
{
        HANDLE hContact;
        int    nAckType;
        int    nAckResult;
        HANDLE hSequence;
        LPARAM pszMessage;
};

typedef enum
{
	SIPE_MIRANDA_INPUT_READ  = 1 << 0,  /**< A read condition.  */
	SIPE_MIRANDA_INPUT_WRITE = 1 << 1   /**< A write condition. */

} sipe_miranda_input_condition;

/** The type of callbacks to handle events on file descriptors, as passed to
 *  sipe_input_add().  The callback will receive the @c user_data passed to
 *  sipe_input_add(), the file descriptor on which the event occurred, and the
 *  condition that was satisfied to cause the callback to be invoked.
 */
typedef void (*sipe_miranda_input_function)(gpointer, gint, sipe_miranda_input_condition);

typedef struct sipe_miranda_sel_entry
{
	int sig;
	HANDLE fd;
	sipe_miranda_input_function func;
	gpointer user_data;
};

#define BUDDIES_FOREACH(list) {                \
	GSList *entry = list;                  \
	while (entry) {                        \
		HANDLE hContact = entry->data; \
		entry = entry->next;
#define BUDDIES_FOREACH_END }}


typedef INT_PTR (*SipSimpleServiceFunc)( SIPPROTO*, WPARAM, LPARAM );
typedef int     (*SipSimpleEventFunc)( SIPPROTO*, WPARAM, LPARAM );
typedef void    (*SipSimpleThreadFunc)( SIPPROTO*, void* );

#define _PVTDATAI(sip) ((struct miranda_sipe_private_data*)sip->sipe_public.backend_private)
#define _PVTDATA ((struct miranda_sipe_private_data*)sip->sipe_public.backend_private)

#define _NI(string) SIPE_DEBUG_INFO( "%s:%s (%d) ##NOT IMPLEMENTED## %s", __FILE__, __FUNCTION__, __LINE__, #string )
#define _NIF() _NI("")

gchar* sipe_miranda_getContactString(const SIPPROTO *pr, HANDLE hContact, const gchar* name);
gchar* sipe_miranda_getString(const SIPPROTO *pr, const gchar* name);
int sipe_miranda_getStaticString(const SIPPROTO *pr, HANDLE hContact, const gchar* valueName, gchar* dest, unsigned dest_len);
DWORD sipe_miranda_getDword( const SIPPROTO *pr, HANDLE hContact, const gchar* name, DWORD* rv);
gboolean sipe_miranda_get_bool(const SIPPROTO *pr, const gchar *name, gboolean defval);

void sipe_miranda_setContactString(const SIPPROTO *pr, HANDLE hContact, const gchar* name, const gchar* value);
void sipe_miranda_setContactStringUtf(const SIPPROTO *pr, HANDLE hContact, const gchar* valueName, const gchar* parValue );
void sipe_miranda_setString(const SIPPROTO *pr, const gchar* name, const gchar* value);
void sipe_miranda_setStringUtf(const SIPPROTO *pr, const gchar* name, const gchar* value);
int sipe_miranda_setWord(const SIPPROTO *pr, HANDLE hContact, const gchar* szSetting, WORD wValue);

struct sipe_miranda_sel_entry* sipe_miranda_input_add(HANDLE fd, sipe_miranda_input_condition cond, sipe_miranda_input_function func, gpointer user_data);
gboolean sipe_miranda_input_remove(struct sipe_miranda_sel_entry *entry);

int SendBroadcast(SIPPROTO *pr, HANDLE hContact,int type,int result,HANDLE hProcess,LPARAM lParam);

gchar* sipe_miranda_eliminate_html(const gchar *string, int len);
unsigned short sipe_miranda_network_get_port_from_fd( HANDLE fd );

/* Plugin interface functions */
int sipe_miranda_SendMsg(SIPPROTO *pr, HANDLE hContact, int flags, const char* msg );
int sipe_miranda_RecvMsg(SIPPROTO *pr, HANDLE hContact, PROTORECVEVENT* pre);

/* Plugin event functions */
int sipe_miranda_buddy_delete(SIPPROTO *pr, HANDLE hContact, LPARAM lParam);