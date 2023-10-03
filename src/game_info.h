/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2023 chciken/Niko
 *
 * This class holds general information about the game.
 * The information is obtained by reading the certain regions
 * of the cartridge header:
 * 0x104-0x133: compulsory Nintendo Logo
 * 0x134-0x143: first 16 characters of the title in upper case ASCII
 * 0x144-0x145: two character ASCII new license code
 * 0x146:       whether the game supports SGB functions
 * 0x147:       cartridge type
 * 0x148:       ROM size
 * 0x149:       RAM size
 * 0x14a:       region code (either japanese or non-japanese)
 * 0x14b:       old license code (0x33 = look at 0x144-0x145)
 *
 * Sources: https://gbdev.gg8.se/wiki/articles/The_Cartridge_Header
 *          https://gbdev.gg8.se/wiki/articles/Gameboy_ROM_Header_Info#Licensee
 ******************************************************************************/
#pragma once

#include <filesystem>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>

#include "utils.h"

class GameInfo {
 public:
  const std::map<string, string> new_license_code_map {
    {"00", "none"},              {"01", "Nintendo R&D1"}, {"08", "Capcom"},
    {"13", "Electronic Arts"},   {"18", "Hudon Soft"},    {"19", "b-ai"},
    {"20", "kss"},               {"22", "pow"},           {"24", "PCM Complete"},
    {"25", "san-x"},             {"28", "Kemco Japan"},   {"29", "seta"},
    {"30", "Viacom"},            {"31", "Nintendo"},      {"32", "Bandai"},
    {"33", "Ocean/Acclaim"},     {"34", "Konami"},        {"35", "Hector"},
    {"37", "Taito"},             {"38", "Hudson"},        {"39", "Banpresto"},
    {"41", "Ubisoft"},           {"42", "Atlus"},         {"44", "Malibu"},
    {"46", "angel"},             {"47", "Bullet-Proof"},  {"49", "irem"},
    {"50", "Absolute"},          {"51", "Acclaim"},       {"52", "Activision"},
    {"53", "American sammy"},    {"54", "Konami"},        {"55", "Hi tech entertainment"},
    {"56", "LJN"},               {"57", "Matchbox"},      {"58", "Mattel"},
    {"59", "Milton Bradley"},    {"60", "Titus"},         {"61", "Virgin"},
    {"64", "LucasArts"},         {"67", "Ocean"},         {"69", "Electronic Arts"},
    {"70", "Infogrames"},        {"71", "Interplay"},     {"72", "Broderbund"},
    {"73", "sculptured"},        {"75", "sci"},           {"78", "THQ"},
    {"79", "Accolade"},          {"80", "misawa"},        {"83", "lozc"},
    {"86", "tokuma shoten"},     {"87", "tsukuda ori"},   {"91", "Chunsoft"},
    {"92", "Video system"},      {"93", "Ocean/Acclaim"}, {"95", "Varie"},
    {"96", "Yonezawa/s'pal"},    {"97", "Kaneko"},        {"99", "Pack in soft"},
    {"96", "Konami (Yu-Gi-Oh!)"}
  };

  const std::map<u8, string> old_license_code_map {
    {0x00, "none"},                  {0x01, "nintendo"},         {0x08, "Capcom"},
    {0x09, "hot-b"},                 {0x0A, "jaleco"},           {0x0B, "coconuts"},
    {0x0C, "elite systems"},         {0x13, "Electronic Arts"},  {0x18, "Hudon Soft"},
    {0x19, "itc entertainment"},     {0x1A, "yanoman"},          {0x1D, "clary"},
    {0x1F, "virgin"},                {0x20, "KSS"},              {0x24, "PCM Complete"},
    {0x25, "san-x"},                 {0x28, "kotobuki systems"}, {0x29, "seta"},
    {0x30, "infogrames"},            {0x31, "Nintendo"},         {0x32, "Bandai"},
    {0x33, "GBC"},                   {0x34, "Konami"},           {0x35, "Hector"},
    {0x38, "Capcom"},                {0x39, "Banpresto"},        {0x3C, "entertainment i"},
    {0x3E, "gremlin"},               {0x41, "Ubisoft"},          {0x42, "Atlus"},
    {0x44, "Malibu"},                {0x46, "angel"},            {0x47, "spectrum holobody"},
    {0x49, "irem"},                  {0x4A, "virgin"},           {0x4D, "Malibu"},
    {0x4f, "U.S. Gold"},             {0x50, "Absolute"},         {0x51, "Acclaim"},
    {0x52, "Activision"},            {0x53, "American sammy"},   {0x54, "Gametek"},
    {0x55, "park place"},            {0x56, "LJN"},              {0x57, "Matchbox"},
    {0x59, "Milton Bradley"},        {0x5A, "mindscape"},        {0x5B, "romstar"},
    {0x5C, "naxat soft"},            {0x5D, "tradewest"},        {0x60, "titus"},
    {0x61, "Virgin"},                {0x67, "Ocean"},            {0x69, "Electronic Arts"},
    {0x6E, "Elite Systems"},         {0x6F, "Electro Brain"},    {0x70, "Infogrammes"},
    {0x71, "Interplay"},             {0x72, "Broderbund"},       {0x73, "sculptured"},
    {0x75, "the sales curve"},       {0x78, "THQ"},              {0x79, "Accolade"},
    {0x7A, "triffix entertainment"}, {0x7C, "micropose"},        {0x7F, "kemco"},
    {0x80, "misawa"},                {0x83, "lozc"},             {0x86, "tokuma shoten"},
    {0x8B, "bullet-proof software"}, {0x8c, "vic tokai"},        {0x8E, "ape"},
    {0x8F, "i'max"},                 {0x91, "chun soft"},        {0x92, "video systen"},
    {0x93, "tsuburava"},             {0x95, "varie"},            {0x96, "yonezawa/s'pal"},
    {0x97, "kaneko"},                {0x99, "arc"},              {0x9A, "nihon bussan"},
    {0x9B, "Tecmo"},                 {0x9C, "imagineer"},        {0x9D, "Banpresto"},
    {0x9F, "nova"},                  {0xA1, "Hori electric"},    {0xA2, "Bandai"},
    {0xA4, "Konami"},                {0xA6, "kawada"},           {0xA7, "takara"},
    {0xA9, "technos japan"},         {0xAA, "broderbund"},       {0xAC, "Toei animation"},
    {0xAD, "toho"},                  {0xAF, "Namco"},            {0xB0, "Acclaim"},
    {0xB1, "ascii or nexoft"},       {0xB2, "Bandai"},           {0xB4, "Enix"},
    {0xB6, "HAL"},                   {0xB7, "SNK"},              {0xB9, "pony canyon"},
    {0xBA, "*culture brain o"},      {0xBB, "Sunsoft"},          {0xBD, "Sony imagesoft"},
    {0xBF, "sammy"},                 {0xC0, "Taito"},            {0xC2, "Kemco"},
    {0xC3, "Squaresoft"},            {0xC4, "tokuma shoten"},    {0xC5, "data east"},
    {0xC6, "tonkin house"},          {0xC8, "koei"},             {0xC9, "ufl"},
    {0xCA, "ultra"},                 {0xCB, "vap"},              {0xCC, "use"},
    {0xCD, "meldac"},                {0xCE, "pony canyon"},      {0xCF, "angel"},
    {0xD0, "Taito"},                 {0xD1, "sofel"},            {0xD2, "quest"},
    {0xD3, "sigma enterprises"},     {0xD4, "ask kodansha"},     {0xD6, "naxat soft"},
    {0xD7, "copya systems"},         {0xD9, "Banpresto"},        {0xDA, "tomy"},
    {0xDB, "ljn"},                   {0xDD, "ncs"},              {0xDE, "human"},
    {0xDF, "altron"},                {0xE0, "jaleco"},           {0xE1, "towachiki"},
    {0xE2, "uutaka"},                {0xE3, "varie"},            {0xE5, "epoch"},
    {0xE7, "athena"},                {0xE8, "asmik"},            {0xE9, "natsume"},
    {0xEA, "king records"},          {0xEB, "atlus"},            {0xEC, "Epic/Sony records"},
    {0xEE, "igs"},                   {0xF0, "a wave"},           {0xF3, "extreme entertainment"},
    {0xFF, "ljn"}
  };

  const std::map<uint, string> cartridge_type_map {
    {0x00, "ROM ONLY"},         {0x01, "MBC1"},          {0x02, "MBC1+RAM"},
    {0x03, "MBC1+BAT+RAM"},     {0x05, "MBC2"},          {0x06, "MBC2+BAT"},
    {0x08, "ROM+RAM"},          {0x09, "ROM+BAT+RAM"},   {0x0B, "MMM01"},
    {0x0C, "MMM01+RAM"},        {0x0D, "MMM01+BAT+RAM"}, {0x0F, "MBC3+BAT+TIM"},
    {0x10, "MBC3+BAT+RAM+TIM"}, {0x11, "MBC3"},          {0x12, "MBC3+RAM"},
    {0x13, "MBC3+BAT+RAM"},     {0x19, "MBC5"},          {0x1A, "MBC5+RAM"},
    {0x1B, "MBC5+BAT+RAM"},     {0x1C, "MBC5+RUM"},      {0x1D, "MBC5+RAM+RUM"},
    {0x1E, "MBC5+BAT+RAM+RUM"}, {0x20, "MBC6"},          {0x22, "MBC7+BAT+RAM+RUM+SEN"},
    {0xFC, "POCKET CAM"},       {0xFD, "BANDAI TAMA5"},  {0xFE, "HuC3"},
    {0xFF, "HuC1+RAM+BAT"}
  };

  const std::map<uint, std::tuple<string, uint>> rom_size_map {
    {0x00, {"32kiB", 0}},              {0x01, {"64kiB (4 banks)", 4}},
    {0x02, {"128kiB (8 banks)", 8}},   {0x03, {"256kiB (16 banks)", 16}},
    {0x04, {"512kiB (32 banks)", 32}}, {0x05, {"1MiB (64 banks)", 64}},
    {0x06, {"2MiB (128 banks)", 128}}, {0x07, {"4MiB (256 banks)", 256}},
    {0x08, {"8MiB (512 banks)", 512}}, {0x52, {"1.1MiB (72 banks)", 72}},
    {0x53, {"1.2MiB (80 banks)", 80}}, {0x54, {"1.5MiB (96 banks)", 96}},
  };

  const std::map<uint, std::tuple<string, uint>> ram_size_map {
    {0x00, {"None", 0}},   {0x01, {"2KiB", 2}},     {0x02, {"8kiB", 8}},
    {0x03, {"32KiB", 32}}, {0x04, {"128kiB", 128}}, {0x05, {"64kiB", 64}}
  };

  const std::map<uint, string> region_map {
    {0x00, "Japanese"}, {0x01, "Non-Japanese"}
  };

  const u16 kAdrTitleStart      = 0x134;
  const u16 kAdrTitleEnd        = 0x143;
  const u16 kAdrNewLicenseStart = 0x144;
  const u16 kAdrNewLicenseEnd   = 0x145;
  const u16 kAdrSgbFunc         = 0x146;
  const u16 kAdrCartType        = 0x147;
  const u16 kAdrRomSize         = 0x148;
  const u16 kAdrRamSize         = 0x149;
  const u16 kAdrRegionCode      = 0x14a;
  const u16 kAdrOldLicense      = 0x14b;

  explicit GameInfo(std::filesystem::path game_path);

  operator string() const;

  bool GetUsesNewLicense();
  bool GetSgbSupport();
  string GetCartridgeType();
  string GetLicenseCode();
  string GetRegion();
  string GetTitle();
  uint GetRamSize();
  uint GetRomSize();

 private:
  bool uses_new_license_;
  bool sgb_support_;
  string cartridge_type_;
  string license_code_;
  string region_;
  string title_;
  std::tuple<string, uint> ram_size_;
  std::tuple<string, uint> rom_size_;
};
