import solver as s
import examples as e

import timeit

s.solve(e.swan)
s.solve(e.duck)
s.solve(e.big)
s.settings['print_possibility_calcs'] = True
s.settings['print_steps'] = True
s.solve(e.small_snowflake)
