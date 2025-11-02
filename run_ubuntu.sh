#!/bin/sh
python generate_data.py && gcc -o main main.c && ./main && python main.py  && python view.py