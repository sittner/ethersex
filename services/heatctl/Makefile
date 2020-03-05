TOPDIR ?= ../..
include $(TOPDIR)/.config

$(HEATCTL_SUPPORT)_ECMD_SRC += \
	services/heatctl/heatctl_ecmd.c

$(HEATCTL_SUPPORT)_SRC += \
	services/heatctl/heatctl.c \

$(HEATCTL_MQTT_SUPPORT)_SRC += \
	services/heatctl/heatctl_mqtt.c \

$(HEATCTL_HOMIE_SUPPORT)_SRC += \
	services/heatctl/heatctl_homie.c \

##############################################################################
# generic fluff
include $(TOPDIR)/scripts/rules.mk
