TOPDIR ?= ../..
include $(TOPDIR)/.config

$(TANKLEVEL_SUPPORT)_ECMD_SRC += \
	services/tanklevel/tanklevel_ecmd.c

$(TANKLEVEL_SUPPORT)_SRC += \
	services/tanklevel/tanklevel.c \

$(TANKLEVEL_HOMIE_SUPPORT)_SRC += \
	services/tanklevel/tanklevel_homie.c \

##############################################################################
# generic fluff
include $(TOPDIR)/scripts/rules.mk
