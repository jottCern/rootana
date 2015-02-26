# This is a file to be run through theta, after running 'create-mllfit-rootfile' and performs 3 steps:
# 1. perform the Bernstein polynomial fit in the Nb=1 channel; uses input from create-mllfit-rootfile output, 'theta.root'.
#    The result is ttbar-bernstein-fit-<channel>.root
# 2. fit the Z peak as a sum of the Bernstein fit from 1. plus a BW convoluted with a Gaussian in the Nb=1 sideband.
#    The ttbar Bernstein fit is used from 1., ttbar-bernstein-fit-<channel>.root; data in the Nb=1 sideband from theta.root.
#    The result is written to bwshape-<channel>.root
# 3. perform the actual fit in the signal region with Nb=2 using the Bernstein shape from 1. and the Z shape from 2.
#    The fit result is written to result-<channel>.root.



execfile('mllfit_include.py')

# step 1: In the N_B = 1 channel, fit the ttbar MC distribution (as 'data') with bernstein polynomials.
# Write the fit result into an output file 'mllfit-<channel>.root'.

nbins_dr = 20

print 'Channel:', channel

theta_channel = '%s_ttbar' % channel

model = build_model_from_rootfile(workdir + 'theta.root', histogram_filter = lambda s: s.startswith(theta_channel + '__'))
procs = model.get_processes(theta_channel)
for p in procs:
    assert p.startswith('b')
    model.get_coeff(theta_channel, p).add_factor('id', parameter = p)
    model.distribution.set_distribution(p, 'gauss', mean = 1.0, width = inf, range = [0.0, inf])

result = mle(model, 'data', 1, signal_process_groups = {'': []}, chi2 = True)['']
print 'bernstein fit chi2/ndof = %.3f / %.3f' % (result['__chi2'][0], model.get_range_nbins(theta_channel)[2] - 4)
parameters = {}
for p in model.get_parameters([]):
    parameters[p] = result[p][0][0]
hists = evaluate_prediction(model, parameters, False)
h = None
for p in hists[theta_channel]:
    if h is None: h = hists[theta_channel][p]
    else: h = h.add(1.0, hists[theta_channel][p])
h = h.scale(1.0 / h.get_value_sum())
hists = {'%s_data__ttbar' % channel: h}
bernstein_ttbar_hist = h
for i in range(nbins_dr):
    hists['%s_dr%d__ttbar' % (channel, i)] = h
write_histograms_to_rootfile(hists, workdir + 'ttbar-bernstein-fit-%s.root' % channel)


# step 2: fit the Z peak in data in the N_B=1 channel with a sum of the background (just fitted) and
# a Z mass peak, modeled with a Breit-Wigner convoluted with a Gauss.


from theta_auto.bw_gauss import *

theta_channel = '%s_data' % channel
files = [workdir + 'ttbar-bernstein-fit-%s.root' % channel, workdir + 'theta.root']
model = build_model_from_rootfile(files, histogram_filter = lambda s: s.startswith(theta_channel + '__'))

hdata = model.get_data_histogram(theta_channel)
ndata = hdata.get_value_sum()

bw = BWGaussHistogramFunction('zmass', 'zwidth', 'res', 90, 60.0, 150.0)

model.set_histogram_function(theta_channel, 'bw_peak', bw)

model.get_coeff(theta_channel, 'bw_peak').add_factor('id', parameter = 'nz')
model.get_coeff(theta_channel, 'ttbar').add_factor('id', parameter = 'nbkg')

model.distribution.set_distribution('zmass', 'gauss', mean = 90.4, width = inf, range = [0, inf])
model.distribution.set_distribution('zwidth', 'gauss', mean = 3.6, width = inf, range = [0, inf])
model.distribution.set_distribution('res', 'gauss', mean = 1.0, width = inf, range = [0, inf])

model.distribution.set_distribution('nz', 'gauss', mean = 0.9 * ndata, width = inf, range = [0, inf])
model.distribution.set_distribution('nbkg', 'gauss', mean = 0.1 * ndata, width = inf, range = [0, inf])

result = mle(model, 'data', 1, signal_process_groups = {'': []}, chi2 = True)
print result['']
res = result['']['res'][0][0]
zmass = result['']['zmass'][0][0]
zwidth = result['']['zwidth'][0][0]
nz = result['']['nz'][0][0]
nbkg = result['']['nbkg'][0][0]

print 'resolution = %.2f, zmass = %.2f, zwidth = %.2f' % (res, zmass, zwidth)
print 'BW peak chi2 = ', result['']['__chi2'][0]

def get_bwshape(zmass, zwidth, res):
    model = Model()
    bw = BWGaussHistogramFunction('zmass', 'zwidth', 'res', 90, 60.0, 150.0)
    model.set_histogram_function('mll', 'bw', bw)
    model.distribution.set_distribution('zmass', 'gauss', mean = zmass, width = 0.0, range = [zmass, zmass])
    model.distribution.set_distribution('zwidth', 'gauss', mean = zwidth, width = 0.0, range = [zwidth, zwidth])
    model.distribution.set_distribution('res', 'gauss', mean = res, width = 0.0, range = [res, res])

    result = mle(model, 'toys-asimov:0.0', 1, signal_process_groups = {'': []}, pred = True)
    bwhist = result['']['__pred_mll'][0]
    assert bwhist.get_xmin() == 60.0 and bwhist.get_xmax() == 150.0 and bwhist.get_nbins() == 90
    return bwhist
    
bwhist = get_bwshape(zmass, zwidth, res)

# write the fitted histogram and the data to output, for plotting:
write_histograms_to_rootfile({'bwshape': bwhist.scale(nz), 'ttbar': model.get_histogram_function(theta_channel, 'ttbar').get_nominal_histo().scale(nbkg),
   'data': model.get_data_histogram(theta_channel)}, workdir + 'bwshape-%s.root' % channel)
   
print 'data in n_b = 1 sideband: ', model.get_data_histogram(theta_channel).get_value_sum()


# get the scale factor to convert the total number of fitted Z to the number of events in the peak region of [71,111]:
vals = bwhist.get_values()
factor = sum(vals[11:51])
bwhist = bwhist.scale(1.0 / factor)
print "fraction of Breit-Wigner contained in m_ll [71,111]: %.3f" % factor

vals = bernstein_ttbar_hist.get_values()
ttfactor = sum(vals[11:51]) / sum(vals)
print "fraction of ttbar Bernstein hist contained in m_ll [71,111]: %.3f" % ttfactor


# step 3: using the bernstein polynomial from step 1 and the parameters of the Z peak from step 2,
# fit the Z purity in each bin of the DR distribution.


chi2_histo = ROOT.TH1D('chi2_nd', 'chi2_nd', nbins_dr, 0, 4)
bkg_histo = ROOT.TH1D('bkg', 'bkg', nbins_dr, 0, 4)
sig_histo = ROOT.TH1D('sig', 'sig', nbins_dr, 0, 4)
bkg_relerr_histo = ROOT.TH1D('bkg_relerr', 'bkg_relerr', nbins_dr, 0, 4)
sig_relerr_histo = ROOT.TH1D('sig_relerr', 'sig_relerr', nbins_dr, 0, 4)

for idr in range(nbins_dr):
    theta_channel = '%s_dr%d' % (channel, idr)
    model = build_model_from_rootfile(files, histogram_filter = lambda s: s.startswith(theta_channel + '__'))
    hf = HistogramFunction()
    hf.set_nominal_histo(bwhist)
    model.set_histogram_function(theta_channel, 'bw_peak', hf)

    model.get_coeff(theta_channel, 'bw_peak').add_factor('id', parameter = 'nz')
    model.get_coeff(theta_channel, 'ttbar').add_factor('id', parameter = 'nbkg')
    model.get_coeff(theta_channel, 'ttbar').add_factor('constant', value = 1.0 / ttfactor)

    model.distribution.set_distribution('nz', 'gauss', mean = 0.9 * ndata, width = inf, range = [0, inf])
    model.distribution.set_distribution('nbkg', 'gauss', mean = 0.1 * ndata, width = inf, range = [0, inf])
    
    result = mle(model, 'data', 1, signal_process_groups = {'': []}, chi2 = True)
    chi2_histo.SetBinContent(idr + 1, result['']['__chi2'][0] / 90.)
    bkg_histo.SetBinContent(idr + 1, result['']['nbkg'][0][0])
    bkg_histo.SetBinError(idr + 1, result['']['nbkg'][0][1])
    sig_histo.SetBinContent(idr + 1, result['']['nz'][0][0])
    sig_histo.SetBinError(idr + 1, result['']['nz'][0][1])
    bkg_relerr_histo.SetBinContent(idr + 1, bkg_histo.GetBinError(idr + 1) / bkg_histo.GetBinContent(idr + 1))
    sig_relerr_histo.SetBinContent(idr + 1, sig_histo.GetBinError(idr + 1) / sig_histo.GetBinContent(idr + 1))

f = ROOT.TFile(workdir + 'result-%s.root' % channel, 'recreate')
for h in (chi2_histo, sig_histo, bkg_histo, sig_relerr_histo, bkg_relerr_histo):
    h.SetDirectory(f)
    h.Write()
f.Close()

