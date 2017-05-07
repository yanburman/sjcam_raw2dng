#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import ConfigParser
import sys
import os


class Preferences(ConfigParser.SafeConfigParser):
    def __init__(self):
        ConfigParser.SafeConfigParser.__init__(self)

        # make config case sensitive
        self.optionxform = str

        self.settings_section = 'Settings'
        self.general_section = 'General'
        defaults = dict()
        defaults[self.settings_section] = {'DNG': True, 'TIFF': False, 'Thumbnail': False, 'Rotate': False}
        defaults[self.general_section] = {'Language': 'en'}

        self.prefs_path = os.path.join(os.path.dirname(sys.argv[0]), 'preferences.ini')
        self.read(self.prefs_path)

        need_to_write = False

        for section in defaults:
            if not self.has_section(section):
                self.add_section(section)
                need_to_write = True
            values = defaults[section]
            for key, val in values.items():
                if not self.has_option(section, key):
                    self.set(section, key, str(val))
                    need_to_write = True

        if need_to_write:
            self.write()

    def write(self):
        with open(self.prefs_path, 'wb') as configfile:
            ConfigParser.SafeConfigParser.write(self, configfile)

    def get_dng(self):
        return self.getboolean(self.settings_section, 'DNG')

    def get_tiff(self):
        return self.getboolean(self.settings_section, 'TIFF')

    def get_thumbnail(self):
        return self.getboolean(self.settings_section, 'Thumbnail')

    def get_rotate(self):
        return self.getboolean(self.settings_section, 'Rotate')

    def get_lang(self):
        return self.get(self.general_section, 'Language')

    def set_dng(self, val):
        self.set(self.settings_section, 'DNG', val)
        self.write()

    def set_tiff(self, val):
        self.set(self.settings_section, 'TIFF', val)
        self.write()

    def set_thumbnail(self, val):
        self.set(self.settings_section, 'Thumbnail', val)
        self.write()

    def set_rotate(self, val):
        self.set(self.settings_section, 'Rotate', val)
        self.write()

    def set_lang(self, val):
        self.set(self.general_section, 'Language')
        self.write()


if __name__ == "__main__":
    prefs = Preferences()
    print prefs.get_dng()
    print prefs.get_tiff()
    print prefs.get_thumbnail()
    print prefs.get_rotate()
    print prefs.get_lang()
