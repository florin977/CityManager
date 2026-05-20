#!/bin/bash

./CityManager/city_manager --role manager --user City_Hub --list $1 | grep -e Inspector: -e Severity
