channel = 'ee'

nbins_dr = 20


# step 1: In the N_B = 1 channel, fit the ttbar MC distribution (as 'data') with bernstein polynomials.
# Write the fit result into an output file 'mllfit-<channel>.root'.

theta_channel = '%s_ttbar' % channel

model = build_model_from_rootfile('theta.root', histogram_filter = lambda s: s.startswith(theta_channel + '__'))
for i in range(4):
    model.get_coeff(theta_channel, 'b%d' % i).add_factor('id', parameter = 'b%d' % i)
    model.distribution.set_distribution('b%d' % i, 'gauss', mean = 1.0, width = inf, range = [0.0, inf])

result = mle(model, 'data', 1, signal_process_groups = {'': []})['']
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
write_histograms_to_rootfile(hists, 'ttbar-bernstein-fit-%s.root' % channel)


# step 2: fit the Z peak in data in the N_B=1 channel with a sum of the background (just fitted) and
# a Z mass peak, modeled with a Breit-Wigner convoluted with a Gauss.


from theta_auto.bw_gauss import *

theta_channel = '%s_data' % channel
files = ['ttbar-bernstein-fit-%s.root' % channel, 'theta.root']
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
res = result['']['res'][0][0]
zmass = result['']['zmass'][0][0]
zwidth = result['']['zwidth'][0][0]

print 'resolution = %.2f, zmass = %.2f, zwidth = %.2f' % (res, zmass, zwidth)

model = Model()
bw = BWGaussHistogramFunction('zmass', 'zwidth', 'res', 90, 60.0, 150.0)
model.set_histogram_function('mll', 'bw', bw)
model.distribution.set_distribution('zmass', 'gauss', mean = zmass, width = 0.0, range = [zmass, zmass])
model.distribution.set_distribution('zwidth', 'gauss', mean = zwidth, width = 0.0, range = [zwidth, zwidth])
model.distribution.set_distribution('res', 'gauss', mean = res, width = 0.0, range = [res, res])

result = mle(model, 'toys-asimov:0.0', 1, signal_process_groups = {'': []}, pred = True)
bwhist = result['']['__pred_mll'][0]
assert bwhist.get_xmin() == 60.0 and bwhist.get_xmax() == 150.0 and bwhist.get_nbins() == 90

# get the scale factor to convert the total number of fitted Z to the number of events in the peak region of [71,111]:
vals = bwhist.get_values()
factor = sum(vals[11:51])
bwhist = bwhist.scale(1.0 / factor)
print "fraction of Breit-Wigner contained in m_ll [71,111]: %.3f" % factor

#write_histograms_to_rootfile(hists, 'test.root')

vals = bernstein_ttbar_hist.get_values()
ttfactor = sum(vals[11:51]) / sum(vals)
print "fraction of ttbar Bernstein hist contained in m_ll [71,111]: %.3f" % ttfactor


# step 3: using the bernstein polynomial from step 1 and the parameters of the Z peak from step 2,
# fit the Z purity in each bin of the DR distribution.

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
    bkg_histo.SetBinContent(idr + 1, result['']['nbkg'][0][0])
    bkg_histo.SetBinError(idr + 1, result['']['nbkg'][0][1])
    sig_histo.SetBinContent(idr + 1, result['']['nz'][0][0])
    sig_histo.SetBinError(idr + 1, result['']['nz'][0][1])
    bkg_relerr_histo.SetBinContent(idr + 1, bkg_histo.GetBinError(idr + 1) / bkg_histo.GetBinContent(idr + 1))
    sig_relerr_histo.SetBinContent(idr + 1, sig_histo.GetBinError(idr + 1) / sig_histo.GetBinContent(idr + 1))

f = ROOT.TFile('result-%s.root' % channel, 'recreate')
for h in (sig_histo, bkg_histo, sig_relerr_histo, bkg_relerr_histo):
    h.SetDirectory(f)
    h.Write()
f.Close()

