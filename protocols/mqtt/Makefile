TOPDIR ?= ../..
include $(TOPDIR)/.config

$(MQTT_SUPPORT)_SRC += protocols/mqtt/mqtt.c
$(MQTT_SUPPORT)_SRC += protocols/mqtt/static_configuration.c

$(MQTT_HOMIE_SUPPORT)_SRC += protocols/mqtt/mqtt_homie.c

##############################################################################
# generic fluff
include $(TOPDIR)/scripts/rules.mk

