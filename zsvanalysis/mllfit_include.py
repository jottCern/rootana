indir = '/nfs/dust/cms/user/ottjoc/omega-out/ntuple-latest/recoplots/'
n_bernstein = 3
mc_as_data = True

# final selection to use for fit; %s is substituted with channel
#final_selection = 'fs_%s_bc2'

final_selection = 'fs_%s_bc2_met'

# final selection + mll cut, to compare MC vs. fit result
final_selection_mll = final_selection + '_mll'

channel="mm"

if mc_as_data:
    if 'met' in final_selection:
        workdir = 'mllfit_met_mcxcheck_n%d/' % n_bernstein
    else:
        workdir = 'mllfit_mcxcheck_n%d/' % n_bernstein
else:
    workdir = 'mllfit_nominal_n%d/' % n_bernstein

open('.mllfit_workdir', 'w').write(workdir)
