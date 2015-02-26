# this script uses the output of recoplots and creates
# a root file for theta with the ttbar background in the Nb=1 sideband and data in the Nb=1 sideband.
# For the ttbar fit, bernstein polynomials are added as histograms.
# So theta.root should be used to perform the Bernstein fit
# in the Nb=1 sideband; this is done as step 1 in mllfit.py, which is to be run after this script.


execfile('mllfit_include.py')

import ROOT, glob, scipy.special


rootfiles = {}
def gethist(fnames, hname):
    global rootfiles
    result = None
    for f in fnames:
        if f not in rootfiles:
            rootfiles[f] = ROOT.TFile.Open(f, 'read')
            if not rootfiles[f].IsOpen():
                raise RuntimeError, "could not open root file '%s'" % f
        result_tmp = rootfiles[f].Get(hname)
        try:
            ROOT.AddressOf(result_tmp)
        except ValueError:
            raise RuntimeError,  "could not find '%s' in file '%s'" % (hname, f)
        if result is None:
            result = result_tmp.Clone()
        else:
            result.Add(result_tmp)
    return result

    
def eval_bernstein(i, n, t):
    assert i <= n
    return scipy.special.binom(n, i) * t**i * (1.0 - t)**(n-i)
    
    
def create_bernstein_hists(n, name_prefix, outdir, nbins, xmin, xmax):
    binwidth = (xmax - xmin) / nbins
    for i in range(n+1):
        bhist = ROOT.TH1D(name_prefix + '%d' % i, name_prefix + '%d' % i, nbins, xmin, xmax)
        bhist.SetDirectory(outdir)
        for k in range(nbins):
            x = xmin + (k + .5) * binwidth
            b = eval_bernstein(i, n, (k + 0.5)/nbins)
            bhist.SetBinContent(k+1, b)
        outdir.cd()
        bhist.Write()

def create_hists(lepton_flavor, outdir):
    top_files = glob.glob(indir + 'ttbar*.root') + glob.glob(indir + 'st*.root')
    mll_tmp = gethist(top_files, 'fs_%s_bc1/mll' % lepton_flavor)
    mll = ROOT.TH1D("%s_ttbar__DATA" % lepton_flavor, "%s_ttbar__DATA" % lepton_flavor, 90, 60, 150)
    mll.SetDirectory(outdir)
    
    def trafo_hist(mll_old, mll_new, scale_for_error = False):
        assert mll_old.GetXaxis().GetBinWidth(1) == 1.0
        assert mll_new.GetXaxis().GetBinWidth(1) == 1.0
        assert mll_new.GetNbinsX() == 90
        assert mll_new.GetXaxis().GetXmin() == 60.0
        nbins_old = mll_old.GetNbinsX()
        for i in range(90):
            ibin_old = mll_old.GetXaxis().FindFixBin(60.5 + i)
            assert ibin_old >= 1
            assert ibin_old <= nbins_old
            mll_new.SetBinContent(i+1, mll_old.GetBinContent(ibin_old))
            mll_new.SetBinError(i+1, mll_old.GetBinError(ibin_old))
        # scale such that Poisson error corresponds to stat. MC error:
        if scale_for_error:
            mll_new.Scale(mll_new.GetEffectiveEntries() / mll_new.Integral())
            
    trafo_hist(mll_tmp, mll, True) # use scale_for_error to get sensible chi2 for Bernstein polynomial fits
    outdir.cd()
    mll.Write()
    
    if mc_as_data:
        data_files = glob.glob(indir + 'ttbar*.root') + glob.glob(indir + 'st*.root') + glob.glob(indir + 'dy*jets*.root')
    else:
        if lepton_flavor == 'mm': data_files = glob.glob(indir + 'dmu_run*.root')
        else:  data_files = glob.glob(indir + 'dele_run*.root')
    #print 'data files: ', data_files
    mll_tmp = gethist(data_files, 'fs_%s_bc1/mll' % lepton_flavor)
    mll = ROOT.TH1D("%s_data__DATA" % lepton_flavor, "%s_data__DATA" % lepton_flavor, 90, 60, 150)
    mll.SetDirectory(outdir)
    trafo_hist(mll_tmp, mll)
    outdir.cd()
    mll.Write()
    
    # in bins of DR, from the 2D hists:
    global final_selection
    mll_dr = gethist(data_files, (final_selection + '/mll_drbb') % lepton_flavor) # x-axis = DR, y = mll
    assert abs(mll_dr.GetXaxis().GetBinWidth(1) - 0.1) < 1e-5
    for i in range(20):
        mll_old = mll_dr.ProjectionY("_py", 2*i+1, 2*i+2) # sum up 2 bin in DR to have 0.2 bin width
        mll_new = ROOT.TH1D("%s_dr%d__DATA" % (lepton_flavor, i), "%s_dr%d__DATA" % (lepton_flavor, i), 90, 60, 150)
        trafo_hist(mll_old, mll_new)
        del mll_old
        outdir.cd()
        #mll_new = mll_old.Clone("%s_dr%d__DATA" % (lepton_flavor, i))
        mll_new.Write()
        
        
def create_bkgsub_hists(channel, outdir):
    bkg_files = glob.glob(indir + 'ttbar*.root') + glob.glob(indir + 'st*.root') + [indir + 'ww.root', indir + 'wz.root', indir + 'zz.root']
    if channel.startswith('mm'):
        data_files = glob.glob(indir + 'dmu_run*.root')
    else:
        assert channel.startswith('ee')
        data_files = glob.glob(indir + 'dele_run*.root')
    drbb_bkg = gethist(bkg_files, (final_selection_mll + '/DR_BB') % channel)
    drbb_sig = gethist(data_files, (final_selection_mll + '/DR_BB') % channel)
    drbb_bkg.Rebin(8)
    drbb_sig.Rebin(8)
    drbb_bkg.SetName('bkg_%s' % channel)
    drbb_sig.SetName('sig_%s' % channel)
    drbb_bkg.Scale(0.92**2)
    drbb_sig.Scale(0.92**2)
    drbb_bkg.SetDirectory(outdir)
    drbb_sig.SetDirectory(outdir)
    drbb_sig.Add(drbb_bkg, -1.0)
    outdir.cd()
    drbb_sig.Write()
    drbb_bkg.Write()
    
    
outfile = ROOT.TFile(workdir + 'theta.root', 'recreate')
create_hists('mm', outfile)
create_hists('ee', outfile)

#create_hists('me', outfile)

create_bernstein_hists(n_bernstein, 'mm_ttbar__b', outfile, 90, 60, 150)
create_bernstein_hists(n_bernstein, 'ee_ttbar__b', outfile, 90, 60, 150)

d = outfile.mkdir('bkgsub')

create_bkgsub_hists('mm', d)
create_bkgsub_hists('ee', d)

#outfile.cd()
#outfile.Write()
outfile.Close()

print 'done'

