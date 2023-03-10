APPLICATION = master
# If no BOARD is found in the environment, use this default:
BOARD ?= arduino-mega2560
# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

USEMODULE += LiFi_module
USEMODULE += xtimer
FEATURES_REQUIRED += periph_adc
FEATURES_REQUIRED += periph_spi
FEATURES_REQUIRED += cpp
#FEATURES_REQUIRED += periph_eeprom
include $(RIOTBASE)/Makefile.include

CFLAGS += -DSS=ARDUINO_PIN_53
CXXEXFLAGS +=-std=c++11
