/*
 * Copyright (C) 2024 Caian Benedicto <caianbene@gmail.com>
 *
 * This file is part of xinfc.
 *
 * xinfc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * xinfc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with xinfc.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "i2c_nfc_device.hpp"
#include "version.hpp"
#include "wifi.hpp"

#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>

int parse_i2c_address(const std::string&);
bool select_encryption_mode(const std::string&, xinfc::wifi_crypt&,
    xinfc::wifi_auth&);
int apply_config(const std::string&, int, const std::string&,
    const std::string&, const xinfc::wifi_crypt&, const xinfc::wifi_auth&);
size_t make_wsc_ndef(const std::string&, const std::string&,
    const xinfc::wifi_crypt&, const xinfc::wifi_auth&, unsigned char*, size_t);
void print_usage();

int main(int argc, const char* argv[])
{
    using namespace xinfc;

    std::cerr << "xinfc version " << XINFC_VERSION << std::endl;

    if (argc != 6)
    {
        print_usage();
        exit(1);
    }

    const std::string i2cbus = argv[1];
    const std::string i2caddr_s = argv[2];
    const std::string ssid = argv[3];
    const std::string pass = argv[4];
    const std::string mode = argv[5];

    if (i2cbus.size() != 1)
    {
        std::cerr
            << "Error: Invalid i2c bus parameter!"
            << std::endl;

        exit(1);
    }

    const int i2caddr = parse_i2c_address(i2caddr_s);

    std::cerr << "I2c device address is " << i2caddr << "." << std::endl;

    if (i2caddr <= 0)
    {
        exit(2);
    }

    if (ssid.size() < wifi_lenghts::ssid_min || ssid.size() > wifi_lenghts::ssid_max)
    {
        std::cerr
            << "Error: ssid must have between " << wifi_lenghts::ssid_min
            << " and " << wifi_lenghts::ssid_max << " characters!"
            << std::endl;

        exit(3);
    }

    if (pass.size() < wifi_lenghts::pass_min || pass.size() > wifi_lenghts::pass_max)
    {
        std::cerr
            << "Error: password must have between " << wifi_lenghts::pass_min
            << " and " << wifi_lenghts::pass_max << " characters!"
            << std::endl;

        exit(4);
    }

    wifi_crypt crypt;
    wifi_auth auth;

    if (!select_encryption_mode(mode, crypt, auth))
    {
        exit(5);
    }

    try
    {
        int r = apply_config(i2cbus, i2caddr, ssid, pass, crypt, auth);

        return r;
    }
    catch(const char* msg)
    {
        std::cerr
            << "Error: " << msg << "!"
            << std::endl;
    }
    catch(const xinfc::i2c_error& error)
    {
        std::cerr
            << "Error: " << error.msg << "! ret=" << error.ret
            << " errno=" << error.eno
            << std::endl;
    }

    return 20;
}

int apply_config(
    const std::string& i2cbus,
    int i2caddr,
    const std::string& ssid,
    const std::string& pass,
    const xinfc::wifi_crypt& crypt,
    const xinfc::wifi_auth& auth
)
{
    std::cerr << "Opening i2c bus " << i2cbus << "..." << std::endl;

    xinfc::i2c_nfc_device device(i2cbus, i2caddr);

    std::cerr << "Setting i2c timeout..." << std::endl;

    device.set_timeout(3);

    std::cerr << "Setting i2c retries..." << std::endl;

    device.set_retries(2);

    std::cerr << "Setting i2c device address " << i2caddr << "..." << std::endl;

    device.set_device_address((unsigned char)i2caddr);

    std::cerr << "I2c device address set." << std::endl;

    ///////////////////////////////////////////////////////

    const std::string backup_filename = "nfc_ndef_backup.bin";

    unsigned char ndef_rbuf[xinfc::i2c_nfc_device::max_ndef_buf_size];
    unsigned char ndef_wbuf[xinfc::i2c_nfc_device::max_ndef_buf_size];

    std::cerr << "Reading existing NDEF data..." << std::endl;

    device.read_ndef(ndef_rbuf, sizeof(ndef_rbuf));

    std::cerr << "Read " << sizeof(ndef_rbuf) << " bytes." << std::endl;

    const auto backup_fd = open(backup_filename.c_str(),
        O_EXCL | O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

    if (backup_fd < 0)
    {
        if (errno != EEXIST)
        {
            std::cerr
                    << "Cannot open " << backup_filename
                    << "! errno=" << errno
                    << std::endl;

            return 11;
        }

        std::cerr << "Backup file found. Skipping backup..." << std::endl;
    }
    else
    {
        std::cerr
            << "Stock chip data backup not found. Backing up..."
            << std::endl;

        const auto to_write = sizeof(ndef_rbuf);
        const auto written = write(backup_fd, ndef_rbuf, sizeof(ndef_rbuf));
        const auto eno = errno;

        close(backup_fd);

        if (written != (ssize_t)to_write)
        {
            std::cerr
                << "Cannot write to " << backup_filename
                << "! errno=" << eno
                << std::endl;

            return 12;
        }

        std::cerr << "Backup complete." << std::endl;
    }

    ////////////////////////////////////////////////////////////////

    std::cerr << "Building new NDEF data..." << std::endl;

    const size_t size = make_wsc_ndef(ssid, pass, crypt,
        auth, ndef_wbuf, sizeof(ndef_wbuf));

    std::cerr << "New NDEF is " << size << " bytes." << std::endl;

    ////////////////////////////////////////////////////////////////

    const int max_retries = 5;
    int retries = 0;

    for (retries = 0; retries < max_retries; retries++)
    {
        std::cerr << "Writing new NDEF data..." << std::endl;

        const unsigned int max_bytes = 4;

        for (unsigned int i = 0; i < size; i += max_bytes)
        {
            const unsigned int s = std::min(max_bytes,
                (unsigned int)size - i);

            const int max_w_retries = 5;
            unsigned int w_retries = 0;

            for (w_retries = 0; w_retries <= max_w_retries; w_retries++)
            {
                try
                {
                    device.write_ndef_at(ndef_wbuf + i, s, i);

                    break;
                }
                catch(const xinfc::i2c_error& error)
                {
                    if (w_retries == max_w_retries)
                    {
                        std::cerr << std::endl;

                        throw;
                    }

                    std::cerr << "x";

                    // The i2c line / device sometimes hang
                    // TODO tune timeout/retries ioctl?
                    usleep(20000);
                }
            }

            std::cerr << ".";
        }

        std::cerr << std::endl;

        usleep(1000000);

        std::cerr << "Verifying written NDEF data..." << std::endl;

        {
            memset(ndef_rbuf, 0, size);

            const unsigned int aligned_size = (((size - 1) / 4) + 1) * 4;

            const int max_w_retries = 20;
            unsigned int w_retries = 0;

            for (w_retries = 0; w_retries <= max_w_retries; w_retries++)
            {
                try
                {
                    device.read_ndef(ndef_rbuf, aligned_size);

                    break;
                }
                catch(const xinfc::i2c_error& error)
                {
                    if (w_retries == max_w_retries)
                    {
                        std::cerr << std::endl;

                        throw;
                    }

                    // The i2c line / device sometimes hang
                    // TODO tune timeout/retries ioctl?
                    usleep(40000);
                }
            }

            if (std::equal(ndef_wbuf, ndef_wbuf + size, ndef_rbuf))
            {
                std::cerr << "Success." << std::endl;

                break;
            }
        }

        std::cerr << "Data does not match! Retrying..." << std::endl;
    }

    if (retries == max_retries)
    {
        throw "failed to write new NDEF data";
    }

    return 0;
}

size_t make_wsc_ndef(
    const std::string& ssid,
    const std::string& pass,
    const xinfc::wifi_crypt& crypt,
    const xinfc::wifi_auth& auth,
    unsigned char* buf,
    size_t max_size
)
{
    const char ndef_app[] = "application/vnd.wfa.wsc";

    if (69 + ssid.size() + pass.size() > max_size)
        return 0;

    const auto payload_len = 35 + ssid.size() + pass.size();

    size_t i = 0;

    buf[i++] = 0x03;
    buf[i++] = 30 + payload_len;
    buf[i++] = 0xd2; buf[i++] = 0x17;
    buf[i++] = 4 + payload_len;

    for (unsigned int j = 0; j < sizeof(ndef_app) - 1; j++)
        buf[i++] = ndef_app[j];

    buf[i++] = 0x10; buf[i++] = 0x0e;     // NDEF credential tag
    buf[i++] = (payload_len >> 8) & 0xFF; // payload length
    buf[i++] =  payload_len       & 0xFF; // payload length
    buf[i++] = 0x10; buf[i++] = 0x26;     // NDEF network idx tag
    buf[i++] = 0x00; buf[i++] = 0x01;     // network idx length
    buf[i++] = 0x01;                      // network idx (always 1)
    buf[i++] = 0x10; buf[i++] = 0x45;     // NDEF network name tag
    buf[i++] = (ssid.size() >> 8) & 0xFF; // ssid length
    buf[i++] =  ssid.size()       & 0xFF; // ssid length

    for (unsigned int j = 0; j < ssid.size(); j++)
        buf[i++] = ssid[j];

    buf[i++] = 0x10; buf[i++] = 0x03;     // NDEF auth type tag
    buf[i++] = 0x00; buf[i++] = 0x02;     // auth type length
    buf[i++] = (auth >> 8) & 0xFF;        // auth
    buf[i++] =  auth       & 0xFF;        // auth
    buf[i++] = 0x10; buf[i++] = 0x0f;     // NDEF crypt type tag
    buf[i++] = 0x00; buf[i++] = 0x02;     // crypt type length
    buf[i++] = (crypt >> 8) & 0xFF;       // crypt
    buf[i++] =  crypt       & 0xFF;       // crypt
    buf[i++] = 0x10; buf[i++] = 0x27;     // NDEF network key tag
    buf[i++] = (pass.size() >> 8) & 0xFF; // pass length
    buf[i++] =  pass.size()       & 0xFF; // pass length

    for (unsigned int j = 0; j < pass.size(); j++)
        buf[i++] = pass[j];

    buf[i++] = 0x10; buf[i++] = 0x20;     // NDEF mac address tag
    buf[i++] = 0x00; buf[i++] = 0x06;     // mac address length
    buf[i++] = 0xFF; buf[i++] = 0xFF;     // mac address
    buf[i++] = 0xFF; buf[i++] = 0xFF;     // mac address
    buf[i++] = 0xFF; buf[i++] = 0xFF;     // mac address
    buf[i++] = 0xFE;

    return i;
}

int parse_i2c_address(
    const std::string& saddr
)
{
    if (saddr.size() != 4 || saddr[0] != '0' ||
        (saddr[1] != 'x' && saddr[1] != 'X'))
    {
        std::cerr
            << "Error: Invalid i2c device!"
            << std::endl;

        return -1;
    }

    int addr = 0;

    for (int i = 0; i < 2; i++)
    {
        int c = (int)saddr[2 + i];

        if      (c >= '0' && c <= '9') c = c - (int)'0';
        else if (c >= 'a' && c <= 'f') c = c - (int)'a' + 10;
        else if (c >= 'A' && c <= 'F') c = c - (int)'A' + 10;
        else
        {
            std::cerr
                << "Error: Invalid i2c device!"
                << std::endl;

            return -2;
        }

        addr = (addr << 4) + c;
    }

    return addr;
}

bool select_encryption_mode(
    const std::string& mode,
    xinfc::wifi_crypt& out_crypt,
    xinfc::wifi_auth& out_auth
)
{
    using namespace xinfc;

    if (mode == wifi_str::none())
    {
        out_crypt = wifi_crypt_none;
        out_auth = wifi_auth_open;
        return true;
    }

    if (mode == wifi_str::sae())
    {
        std::cerr
            << "Error: WPA3 encryption modes not supported!"
            << std::endl;

        return false;
    }

    if (mode == wifi_str::sae_mixed())
    {
        std::cerr
            << "Warning: Mixed WPA2/WPA3 will be announced as WPA2!"
            << std::endl;

        out_crypt = wifi_crypt_aes;
        out_auth = wifi_auth_wpa2_personal;
        return true;
    }

    if (mode == wifi_str::psk2_tkip_ccmp())
    {
        out_crypt = wifi_crypt_tkip_aes;
        out_auth = wifi_auth_wpa2_personal;
        return true;
    }

    if (mode == wifi_str::psk2_tkip_aes())
    {
        out_crypt = wifi_crypt_tkip_aes;
        out_auth = wifi_auth_wpa2_personal;
        return true;
    }

    if (mode == wifi_str::psk2_tkip())
    {
        out_crypt = wifi_crypt_tkip;
        out_auth = wifi_auth_wpa2_personal;
        return true;
    }

    if (mode == wifi_str::psk2_ccmp())
    {
        out_crypt = wifi_crypt_aes;
        out_auth = wifi_auth_wpa2_personal;
        return true;
    }

    if (mode == wifi_str::psk2_aes())
    {
        out_crypt = wifi_crypt_aes;
        out_auth = wifi_auth_wpa2_personal;
        return true;
    }

    if (mode == wifi_str::psk2())
    {
        out_crypt = wifi_crypt_aes;
        out_auth = wifi_auth_wpa2_personal;
        return true;
    }

    if (mode == wifi_str::psk_tkip_ccmp())
    {
        out_crypt = wifi_crypt_tkip_aes;
        out_auth = wifi_auth_wpa_personal;
        return true;
    }

    if (mode == wifi_str::psk_tkip_aes())
    {
        out_crypt = wifi_crypt_tkip_aes;
        out_auth = wifi_auth_wpa_personal;
        return true;
    }

    if (mode == wifi_str::psk_tkip())
    {
        out_crypt = wifi_crypt_tkip;
        out_auth = wifi_auth_wpa_personal;
        return true;
    }

    if (mode == wifi_str::psk_ccmp())
    {
        out_crypt = wifi_crypt_aes;
        out_auth = wifi_auth_wpa_personal;
        return true;
    }

    if (mode == wifi_str::psk_aes())
    {
        out_crypt = wifi_crypt_aes;
        out_auth = wifi_auth_wpa_personal;
        return true;
    }

    if (mode == wifi_str::psk())
    {
        out_crypt = wifi_crypt_aes;
        out_auth = wifi_auth_wpa_personal;
        return true;
    }

    if (mode == wifi_str::psk_mixed_tkip_ccmp())
    {
        out_crypt = wifi_crypt_tkip_aes;
        out_auth = wifi_auth_wpa_wpa2_personal;
        return true;
    }

    if (mode == wifi_str::psk_mixed_tkip_aes())
    {
        out_crypt = wifi_crypt_tkip_aes;
        out_auth = wifi_auth_wpa_wpa2_personal;
        return true;
    }

    if (mode == wifi_str::psk_mixed_tkip())
    {
        out_crypt = wifi_crypt_tkip;
        out_auth = wifi_auth_wpa_wpa2_personal;
        return true;
    }

    if (mode == wifi_str::psk_mixed_ccmp())
    {
        out_crypt = wifi_crypt_aes;
        out_auth = wifi_auth_wpa_wpa2_personal;
        return true;
    }

    if (mode == wifi_str::psk_mixed_aes())
    {
        out_crypt = wifi_crypt_aes;
        out_auth = wifi_auth_wpa_wpa2_personal;
        return true;
    }

    if (mode == wifi_str::psk_mixed())
    {
        out_crypt = wifi_crypt_aes;
        out_auth = wifi_auth_wpa_wpa2_personal;
        return true;
    }

    if (mode == wifi_str::wep())
    {
        out_crypt = wifi_crypt_wep;
        out_auth = wifi_auth_open;
        return true;
    }

    if (mode == wifi_str::wep_open())
    {
        out_crypt = wifi_crypt_wep;
        out_auth = wifi_auth_open;
        return true;
    }

    if (mode == wifi_str::wep_shared())
    {
        out_crypt = wifi_crypt_wep;
        out_auth = wifi_auth_shared;
        return true;
    }

    if (mode == wifi_str::wpa3())
    {
        std::cerr
            << "Error: WPA3 encryption modes not supported!"
            << std::endl;

        return false;
    }

    if (mode == wifi_str::wpa3_mixed())
    {
        std::cerr
            << "Warning: Mixed WPA2/WPA3 will be announced as WPA2!"
            << std::endl;

        out_crypt = wifi_crypt_aes;
        out_auth = wifi_auth_wpa2_enterprise;
        return true;
    }

    if (mode == wifi_str::wpa2_tkip_ccmp())
    {
        out_crypt = wifi_crypt_tkip_aes;
        out_auth = wifi_auth_wpa2_enterprise;
        return true;
    }

    if (mode == wifi_str::wpa2_tkip_aes())
    {
        out_crypt = wifi_crypt_tkip_aes;
        out_auth = wifi_auth_wpa2_enterprise;
        return true;
    }

    if (mode == wifi_str::wpa2_ccmp())
    {
        out_crypt = wifi_crypt_aes;
        out_auth = wifi_auth_wpa2_enterprise;
        return true;
    }

    if (mode == wifi_str::wpa2_aes())
    {
        out_crypt = wifi_crypt_aes;
        out_auth = wifi_auth_wpa2_enterprise;
        return true;
    }

    if (mode == wifi_str::wpa2())
    {
        out_crypt = wifi_crypt_aes;
        out_auth = wifi_auth_wpa2_enterprise;
        return true;
    }

    if (mode == wifi_str::wpa2_tkip())
    {
        out_crypt = wifi_crypt_tkip;
        out_auth = wifi_auth_wpa2_enterprise;
        return true;
    }

    if (mode == wifi_str::wpa_tkip_ccmp())
    {
        out_crypt = wifi_crypt_tkip_aes;
        out_auth = wifi_auth_wpa_enterprise;
        return true;
    }

    if (mode == wifi_str::wpa_tkip_aes())
    {
        out_crypt = wifi_crypt_tkip_aes;
        out_auth = wifi_auth_wpa_enterprise;
        return true;
    }

    if (mode == wifi_str::wpa_ccmp())
    {
        out_crypt = wifi_crypt_aes;
        out_auth = wifi_auth_wpa_enterprise;
        return true;
    }

    if (mode == wifi_str::wpa_aes())
    {
        out_crypt = wifi_crypt_aes;
        out_auth = wifi_auth_wpa_enterprise;
        return true;
    }

    if (mode == wifi_str::wpa_tkip())
    {
        out_crypt = wifi_crypt_tkip;
        out_auth = wifi_auth_wpa_enterprise;
        return true;
    }

    if (mode == wifi_str::wpa())
    {
        out_crypt = wifi_crypt_aes;
        out_auth = wifi_auth_wpa_enterprise;
        return true;
    }

    if (mode == wifi_str::wpa_mixed_tkip_ccmp())
    {
        std::cerr
            << "Warning: Mixed WPA/WPA2 will be announced as WPA2!"
            << std::endl;

        out_crypt = wifi_crypt_tkip_aes;
        out_auth = wifi_auth_wpa2_enterprise;
        return true;
    }

    if (mode == wifi_str::wpa_mixed_tkip_aes())
    {
        std::cerr
            << "Warning: Mixed WPA/WPA2 will be announced as WPA2!"
            << std::endl;

        out_crypt = wifi_crypt_tkip_aes;
        out_auth = wifi_auth_wpa2_enterprise;
        return true;
    }

    if (mode == wifi_str::wpa_mixed_tkip())
    {
        std::cerr
            << "Warning: Mixed WPA/WPA2 will be announced as WPA2!"
            << std::endl;

        out_crypt = wifi_crypt_tkip;
        out_auth = wifi_auth_wpa2_enterprise;
        return true;
    }

    if (mode == wifi_str::wpa_mixed_ccmp())
    {
        std::cerr
            << "Warning: Mixed WPA/WPA2 will be announced as WPA2!"
            << std::endl;

        out_crypt = wifi_crypt_aes;
        out_auth = wifi_auth_wpa2_enterprise;
        return true;
    }

    if (mode == wifi_str::wpa_mixed_aes())
    {
        std::cerr
            << "Warning: Mixed WPA/WPA2 will be announced as WPA2!"
            << std::endl;

        out_crypt = wifi_crypt_aes;
        out_auth = wifi_auth_wpa2_enterprise;
        return true;
    }

    if (mode == wifi_str::wpa_mixed())
    {
        std::cerr
            << "Warning: Mixed WPA/WPA2 will be announced as WPA2!"
            << std::endl;

        out_crypt = wifi_crypt_aes;
        out_auth = wifi_auth_wpa2_enterprise;
        return true;
    }

    if (mode == wifi_str::owe())
    {
        std::cerr
            << "Error: OWE mode not supported!"
            << std::endl;

        return false;
    }

    std::cerr
        << "Error: unknown encryption mode!"
        << std::endl;

    return false;
}

void print_usage()
{
    using namespace xinfc;

    std::cerr
        << "USAGE: xinfcw i2c-bus i2c-device ssid password mode" << std::endl
        << "  i2c-device must be a hex byte in the format 0xMN" << std::endl
        << "  ssid must have between " << wifi_lenghts::ssid_min
        << " and " << wifi_lenghts::ssid_max << " characters." << std::endl
        << "  password must have between "  << wifi_lenghts::pass_min
        << " and " << wifi_lenghts::pass_max << " characters." << std::endl
        << "  mode must be one of the following:" << std::endl
        << "    " << wifi_str::none() << std::endl
        << "    " << wifi_str::sae_mixed() << std::endl
        << "    " << wifi_str::psk2_tkip_ccmp() << std::endl
        << "    " << wifi_str::psk2_tkip_aes() << std::endl
        << "    " << wifi_str::psk2_tkip() << std::endl
        << "    " << wifi_str::psk2_ccmp() << std::endl
        << "    " << wifi_str::psk2_aes() << std::endl
        << "    " << wifi_str::psk2() << std::endl
        << "    " << wifi_str::psk_tkip_ccmp() << std::endl
        << "    " << wifi_str::psk_tkip_aes() << std::endl
        << "    " << wifi_str::psk_tkip() << std::endl
        << "    " << wifi_str::psk_ccmp() << std::endl
        << "    " << wifi_str::psk_aes() << std::endl
        << "    " << wifi_str::psk() << std::endl
        << "    " << wifi_str::psk_mixed_tkip_ccmp() << std::endl
        << "    " << wifi_str::psk_mixed_tkip_aes() << std::endl
        << "    " << wifi_str::psk_mixed_tkip() << std::endl
        << "    " << wifi_str::psk_mixed_ccmp() << std::endl
        << "    " << wifi_str::psk_mixed_aes() << std::endl
        << "    " << wifi_str::psk_mixed() << std::endl
        << "    " << wifi_str::wep() << std::endl
        << "    " << wifi_str::wep_open() << std::endl
        << "    " << wifi_str::wep_shared() << std::endl
        << "    " << wifi_str::wpa3_mixed() << std::endl
        << "    " << wifi_str::wpa2_tkip_ccmp() << std::endl
        << "    " << wifi_str::wpa2_tkip_aes() << std::endl
        << "    " << wifi_str::wpa2_ccmp() << std::endl
        << "    " << wifi_str::wpa2_aes() << std::endl
        << "    " << wifi_str::wpa2() << std::endl
        << "    " << wifi_str::wpa2_tkip() << std::endl
        << "    " << wifi_str::wpa_tkip_ccmp() << std::endl
        << "    " << wifi_str::wpa_tkip_aes() << std::endl
        << "    " << wifi_str::wpa_ccmp() << std::endl
        << "    " << wifi_str::wpa_aes() << std::endl
        << "    " << wifi_str::wpa_tkip() << std::endl
        << "    " << wifi_str::wpa() << std::endl
        << "    " << wifi_str::wpa_mixed_tkip_ccmp() << std::endl
        << "    " << wifi_str::wpa_mixed_tkip_aes() << std::endl
        << "    " << wifi_str::wpa_mixed_tkip() << std::endl
        << "    " << wifi_str::wpa_mixed_ccmp() << std::endl
        << "    " << wifi_str::wpa_mixed_aes() << std::endl
        << "    " << wifi_str::wpa_mixed() << std::endl;
}
