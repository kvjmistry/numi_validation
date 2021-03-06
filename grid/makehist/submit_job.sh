# Notes, it took about 15 mins to copy over 10 files
# Files sizes for 500k POT are around 1GB per file, so refrain from submitting > 25 files_per job otherwise more disk space will need to be requested.
# This can be avoided if you manage to get xrootd streaming to work! I just cant seem to get it working for some unknown reason.

# Jobs are set to run offsite only, for some weird reason the onsite jobs give a strange error relating to the c++ standard libraries (probably due to the hacky way I am setting up the environment.) The most proabable fix to this issue is to remove the environmental config setup for onsite jobs. Saying that, they seem to be running perfectly fine with offsite, so I will just leave it as it is for now. 

# Full options
python ProcessMakehist.py --n_jobs=1 --files_per_job=10 --run=0 --process_shift=0 --memory=700 --lifetime=3 --hpset=0 --horn='FHC'

python ProcessMakehist.py --n_jobs=50 --files_per_job=10 --process_shift=0 --memory=700 --lifetime=3 --run=0

# see the python script for help on these options
python ProcessMakehist.py --n_jobs=1 --files_per_job=10 --run=0

# Use this for other run numbers
python ProcessMakehist.py --n_jobs=50 --files_per_job=10 --memory=700 --lifetime=3 --run=1

# Use this for run 24 which is not a variation, but the cv will all the different variations
python ProcessMakehist.py --n_jobs=100 --files_per_job=5 --process_shift=0 --memory=700 --lifetime=6 --run=24



# Production submissions

# FHC ----------------------------------------------------------------------------------------------------------------

# CV set 1
python ProcessMakehist.py --n_jobs=490 --files_per_job=1 --run=0 --process_shift=0 --memory=900 --lifetime=6 --hpset=1 --horn='FHC' --flist="/pnfs/uboone/persistent/users/kmistry/PPFX/makehist/FHC/files_run0.list"

# CV set 2
python ProcessMakehist.py --n_jobs=490 --files_per_job=1 --run=0 --process_shift=0 --memory=900 --lifetime=6 --hpset=2 --horn='FHC' --flist="/pnfs/uboone/persistent/users/kmistry/PPFX/makehist/FHC/files_run0_set2.list"

# CV set 3
python ProcessMakehist.py --n_jobs=499 --files_per_job=1 --run=0 --process_shift=0 --memory=900 --lifetime=6 --hpset=3 --horn='FHC' --flist="/pnfs/uboone/persistent/users/kmistry/PPFX/makehist/FHC/files_run0_set3.list"

# RHC ----------------------------------------------------------------------------------------------------------------

# CV set 1
python ProcessMakehist.py --n_jobs=492 --files_per_job=1 --run=0 --process_shift=0 --memory=900 --lifetime=6 --hpset=1 --horn='RHC' --flist="/pnfs/uboone/persistent/users/kmistry/PPFX/makehist/RHC/files_run0.list"

# CV set 2
python ProcessMakehist.py --n_jobs=495 --files_per_job=1 --run=0 --process_shift=0 --memory=900 --lifetime=6 --hpset=2 --horn='RHC' --flist="/pnfs/uboone/persistent/users/kmistry/PPFX/makehist/RHC/files_run0_set2.list"

# CV set 3
python ProcessMakehist.py --n_jobs=494 --files_per_job=1 --run=0 --process_shift=0 --memory=900 --lifetime=6 --hpset=3 --horn='RHC' --flist="/pnfs/uboone/persistent/users/kmistry/PPFX/makehist/RHC/files_run0_set3.list"
