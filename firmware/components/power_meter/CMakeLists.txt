idf_component_register(
    SRCS
        "power_meter.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        driver
)

# When idf-hlw8032 is available as a managed component,
# add it to REQUIRES above: "idf-hlw8032"
#
# If using the manual clone approach:
#   PRIV_REQUIRES idf-hlw8032
