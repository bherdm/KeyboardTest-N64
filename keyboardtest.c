/**
 * @file keyboardtest.c
 * @author Christopher Bonhage (me@christopherbonhage.com)
 * @brief N64 test ROM for keyboards: full-size keyboards through BlueRetroForever,
 *        read by libdragon's Keyboard subsystem
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <libdragon.h>

/* How many of the latest key events the screen shows */
#define EVENT_LOG_SIZE 8

const char * format_controller_identifier( uint16_t identifier )
{
    switch( identifier )
    {
        case JOYBUS_IDENTIFIER_NONE:                 return "None        ";
        case JOYBUS_IDENTIFIER_N64_CONTROLLER:       return "Controller  ";
        case JOYBUS_IDENTIFIER_N64_VOICE_RECOGNITION: return "VRU         ";
        case JOYBUS_IDENTIFIER_N64_MOUSE:            return "Mouse       ";
        case JOYBUS_IDENTIFIER_N64_RANDNET_KEYBOARD: return "Keyboard    ";
        default:                                     return "Unsupported ";
    }
}

/* The layout and arrangement BlueRetroForever is set to for a keyboard */
static const char * format_layout( keyboard_layout_t layout )
{
    static const char * const names[] = { "US", "UK", "DE", "FR", "JP" };
    if( layout == KEYBOARD_LAYOUT_NONE ) return "--";
    return (unsigned)layout < sizeof(names) / sizeof(names[0]) ? names[layout] : "??";
}

static const char * format_arrangement( keyboard_arrangement_t arrangement )
{
    switch( arrangement )
    {
        case KEYBOARD_ARRANGEMENT_MAC: return "Mac";
        case KEYBOARD_ARRANGEMENT_PC:  return "PC";
        default:                       return "--";
    }
}

/* BlueRetroForever's state, read through its command 0x7B on the first port
 * with a device that answers it; nothing is changed. Returns the port, or -1. */
static int brf_read( const uint16_t identifiers[JOYPAD_PORT_COUNT], brf_status_t * status,
                           brf_keyboard_settings_t settings[JOYPAD_PORT_COUNT], brf_result_t * kb_result )
{
    JOYPAD_PORT_FOREACH( port )
    {
        if( identifiers[port] == JOYBUS_IDENTIFIER_NONE ) continue;
        if( brf_get_status( port, status ) != BRF_OK ) continue;
        *kb_result = brf_get_keyboard_settings( port, settings );
        return port;
    }
    return -1;
}

int main(void)
{
    /* Initialize peripherals */
    display_init(RESOLUTION_320x240, DEPTH_32_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE);
    console_init();
    debug_init_usblog();    /* route debugf()/stderr to the SC64 USB debug lane (sc64deployer debug).
                             * NOT debug_init_isviewer(): its one-shot boot probe fails on a real
                             * SC64 because the deployer's --isv listener starts only AFTER the ROM
                             * boots; the USB channel just needs the cart present. */
    keyboard_init();        /* starts libdragon's Joybus device detection too, which the [id] lines read */

    console_set_render_mode(RENDER_MANUAL);

    uint16_t identifiers[JOYPAD_PORT_COUNT];
    uint16_t prev_id[JOYPAD_PORT_COUNT];
    bool prev_id_seen = false;
    brf_status_t br_status = {0};
    brf_keyboard_settings_t br_settings[JOYPAD_PORT_COUNT] = {0};
    brf_result_t br_kb_result = BRF_NOT_PRESENT;
    int br_port = -1;
    keyboard_event_t event_log[EVENT_LOG_SIZE];
    int event_count = 0;
    uint32_t frame = 0;

    /* Main loop test */
    while(1)
    {
        /* Each port's device, from libdragon's background detection (once a second).
         * Identifier changes go to the SC64 USB debug lane as they happen. */
        JOYPAD_PORT_FOREACH( port )
        {
            identifiers[port] = joybus_get_identifier( port, NULL );
            if( !prev_id_seen || identifiers[port] != prev_id[port] )
            {
                debugf( "[id] port%u %s (0x%04X)\n", (unsigned)(port + 1),
                        format_controller_identifier( identifiers[port] ), identifiers[port] );
            }
            prev_id[port] = identifiers[port];
        }
        prev_id_seen = true;

        /* ~1Hz heartbeat, so the host-side SC64 `debug` capture stays alive between keystrokes. */
        if( (frame % 60) == 0 )
        {
            debugf( "[hb] frame=%lu P1=%04X P2=%04X P3=%04X P4=%04X\n", (unsigned long)frame,
                    identifiers[0], identifiers[1], identifiers[2], identifiers[3] );

            /* BlueRetroForever's state once a second, logged when it changes */
            brf_status_t status = {0};
            brf_keyboard_settings_t settings[JOYPAD_PORT_COUNT] = {0};
            brf_result_t kb_result = BRF_NOT_PRESENT;
            int port = brf_read( identifiers, &status, settings, &kb_result );
            if( port != br_port || memcmp( &status, &br_status, sizeof(status) ) != 0 ||
                kb_result != br_kb_result || memcmp( settings, br_settings, sizeof(settings) ) != 0 )
            {
                if( port < 0 ) debugf( "[br] none\n" );
                else
                {
                    debugf( "[br] via port%d v%u pairing=%d led=%u devices=%d %d %d %d\n", port + 1,
                            status.version, status.pairing, status.led_level,
                            status.device[0], status.device[1], status.device[2], status.device[3] );
                    if( kb_result != BRF_OK ) debugf( "[br] keyboard settings: result %d\n", kb_result );
                    else JOYPAD_PORT_FOREACH( p )
                        debugf( "[br] port%u %s %s %s%s\n", (unsigned)(p + 1), settings[p].classic ? "classic" : "full",
                                format_layout( settings[p].layout ), format_arrangement( settings[p].arrangement ),
                                settings[p].temporary ? " until-power-off" : "" );
                }
                br_port = port; br_status = status; br_kb_result = kb_result;
                memcpy( br_settings, settings, sizeof(settings) );
            }
        }
        frame++;

        /* Every key that went down or up since the last frame, in order. Caps Lock
         * and Num Lock toggle their LEDs: what a key does is the program's to decide. */
        keyboard_event_t event;
        while( keyboard_get_event( &event ) )
        {
            const char * name = keyboard_key_name( event.key );
            debugf( "[ev] port%u %s %04x %s t=%lu\n", (unsigned)(event.port + 1),
                    event.pressed ? "down" : "up", event.key, name ? name : "?",
                    (unsigned long)get_ticks_ms() );

            if( event.pressed && event.key == KEYBOARD_KEY_CAPSLOCK )
                keyboard_set_leds( event.port, keyboard_get_leds( event.port ) ^ KEYBOARD_LED_CAPS_LOCK );
            if( event.pressed && event.key == KEYBOARD_KEY_NUMLOCKCLEAR )
                keyboard_set_leds( event.port, keyboard_get_leds( event.port ) ^ KEYBOARD_LED_NUM_LOCK );
            if( event.pressed && event.key == KEYBOARD_KEY_SCROLLLOCK )
                keyboard_set_leds( event.port, keyboard_get_leds( event.port ) ^ KEYBOARD_LED_SCROLL_LOCK );

            if( event_count == EVENT_LOG_SIZE )
            {
                for( int i = 1; i < EVENT_LOG_SIZE; i++ ) event_log[i - 1] = event_log[i];
                event_count--;
            }
            event_log[event_count++] = event;
        }

        console_clear();

        printf("Keyboard Test ROM\n\n");

        JOYPAD_PORT_FOREACH( port )
        {
            printf("Controller Port %d: %s (0x%04X)\n", port + 1,
                   format_controller_identifier( identifiers[port] ), identifiers[port]);
        }

        if( br_port < 0 ) printf("\nBlueRetroForever: none\n");
        else printf("\nBlueRetroForever v%u: pairing %s, LED %u\n", br_status.version,
                    br_status.pairing ? "open" : "closed", br_status.led_level);

        printf("\n");
        printf("Keyboard  LEDs  Layout  Held\n");

        JOYPAD_PORT_FOREACH( port )
        {
            if( !keyboard_is_connected( port ) ) continue;

            keyboard_keys_t keys = keyboard_get_keys( port );
            uint8_t leds = keyboard_get_leds( port );
            keyboard_layout_t layout = keyboard_get_layout( port );
            keyboard_arrangement_t arrangement = keyboard_get_arrangement( port );
            int held = 0;
            while( held < KEYBOARD_KEYS_MAX && keys.key[held] ) held++;

            printf( "Port %d     %02x   %s %-3s  %2d%s\n", port + 1, leds, format_layout( layout ),
                    format_arrangement( arrangement ), held, keys.overflow ? "+" : "" );
            printf( " " );
            for( int i = 0; i < held; i++ )
            {
                const char * name = keyboard_key_name( keys.key[i] );
                printf( " %s", name ? name : "?" );
            }
            printf( "\n" );

            /* Mirror keyboard state to the SC64 USB debug lane, edge-triggered so the
             * bounded SC64 `debug` capture neither floods at 60fps nor loses the signal. */
            static keyboard_keys_t prev_keys[4];
            static uint8_t prev_leds[4];
            static keyboard_layout_t prev_layout[4];
            static keyboard_arrangement_t prev_arrangement[4];
            static bool prev_seen[4];
            if( !prev_seen[port] || memcmp( &keys, &prev_keys[port], sizeof(keys) ) != 0 ||
                leds != prev_leds[port] || layout != prev_layout[port] || arrangement != prev_arrangement[port] )
            {
                char list[KEYBOARD_KEYS_MAX * 5 + 1] = "";
                for( int i = 0; i < held; i++ ) sprintf( &list[i * 5], " %04x", keys.key[i] );
                debugf( "[kb] port%u LEDs=%02x layout=%s arrangement=%s held=%d overflow=%d keys=%s\n",
                        (unsigned)(port + 1), leds, format_layout( layout ), format_arrangement( arrangement ),
                        held, keys.overflow, held ? &list[1] : "-" );
                prev_keys[port] = keys; prev_leds[port] = leds; prev_layout[port] = layout;
                prev_arrangement[port] = arrangement; prev_seen[port] = true;
            }
        }

        printf("\n");
        printf("Latest keys:\n");
        for( int i = 0; i < event_count; i++ )
        {
            const char * name = keyboard_key_name( event_log[i].key );
            printf("  Port %d  %-4s  %04x  %s\n", event_log[i].port + 1,
                   event_log[i].pressed ? "down" : "up", event_log[i].key, name ? name : "?");
        }

        console_render();
    }
}
