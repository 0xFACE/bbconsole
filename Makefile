BLUEZ_PATH=../bluez

BLUEZ_SRCS  = lib/bluetooth.c lib/hci.c lib/sdp.c lib/uuid.c
BLUEZ_SRCS += attrib/att.c attrib/gatt.c attrib/gattrib.c attrib/utils.c
BLUEZ_SRCS += btio/btio.c src/log.c src/shared/mgmt.c  
BLUEZ_SRCS += src/shared/crypto.c src/shared/att.c src/shared/queue.c src/shared/util.c
BLUEZ_SRCS += src/shared/io-glib.c src/shared/timeout-glib.c

IMPORT_SRCS = $(addprefix $(BLUEZ_PATH)/, $(BLUEZ_SRCS))
LOCAL_SRCS  = bbconsole.c

CC = gcc
CFLAGS = -Os -g -Wall 

CPPFLAGS = -DHAVE_CONFIG_H
ifneq ($(DEBUGGING),)
CFLAGS += -DBLUEPY_DEBUG=1
endif

CPPFLAGS += -I$(BLUEZ_PATH)/attrib -I$(BLUEZ_PATH) -I$(BLUEZ_PATH)/lib -I$(BLUEZ_PATH)/src -I$(BLUEZ_PATH)/gdbus -I$(BLUEZ_PATH)/btio

CPPFLAGS += $(shell pkg-config glib-2.0 --cflags)
LDLIBS += $(shell pkg-config glib-2.0 --libs)

all: bbconsole

bbconsole: $(LOCAL_SRCS) $(IMPORT_SRCS)
	$(CC) -L. $(CFLAGS) $(CPPFLAGS) -o $@ $(LOCAL_SRCS) $(IMPORT_SRCS) $(LDLIBS)

clean:
	rm -rf *.o bbconsole



