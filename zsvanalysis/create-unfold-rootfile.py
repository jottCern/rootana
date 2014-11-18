import ROOT, glob

indir = '/nfs/dust/cms/user/ottjoc/omega-out/ntuple-latest/response/'

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
        if result_tmp is None: raise RuntimeError, "could not find '%s' in file '%s'" % (hname, f)
        if result is None:
            result = result_tmp.Clone()
        else:
            result.Add(result_tmp)
    print 'Files: %s' % str(fnames)
    if result.GetDimension() == 2:
        integ = result.Integral(0, -1, 0, -1)
    else:
        integ = result.Integral(0, -1)
    print '   Histogram %s has integral %.3f' % (hname, integ)
    return result


# lepton_flavor should be 'mm', 'ee', 'mm_met', or 'ee_met'
# signal should be the name of the BB signal sample: 'dy', 'dybbm', or 'dybb4f' (as DY 'internal' background, always dy will be used).
def create_hists(lepton_flavor, signal, outfile):
    if signal == 'dy':
        signal_files = glob.glob(indir + 'dy[1-4]jetsbbvis.root') + glob.glob(indir + 'dybbvis.root')
        assert len(signal_files) == 5
    elif signal == 'dybbm':
        signal_files = [ indir + 'dybbmbbvis.root' ]
    else:
        assert signal == 'dybb4f'
        signal_files = [ indir + 'dybb4fbbvis.root' ]
    if lepton_flavor.startswith('ee'):
        data_files = glob.glob(indir + 'dele_*.root')
    else:
        assert lepton_flavor.startswith('mm')
        data_files = glob.glob(indir + 'dmu_*.root')
    assert len(data_files) == 4
    # internal background first:
    bkg_files = []
    for q in ('cc', 'bbother', 'other'):
        bkg_files += glob.glob(indir + 'dy*jets%s.root' % q) + glob.glob(indir + 'dy%s.root' % q)
    assert len(bkg_files) == 12 + 3
    bkg_files = bkg_files + glob.glob(indir + 'ttbar*.root')
    response_tmp = gethist(signal_files, 'response_%s/reco_gen_dr' % lepton_flavor)
    response_bkg_tmp = gethist(bkg_files, 'response_%s/reco_gen_dr' % lepton_flavor)
    response_data_tmp = gethist(data_files, 'response_%s/reco_gen_dr' % lepton_flavor)
    gen_tmp = gethist(signal_files, 'response_%s/genonly_dr' % lepton_flavor)

    # make response matrix from response_tmp by dividing by gen hist, such that response contains probabilities as expected by unfolding.
    nbins_gen = 20
    nbins_rec = 20
    
    outdir = outfile.mkdir('%s_%s' % (lepton_flavor, signal))
    outdir.cd()

    response = ROOT.TH2D('response', 'response', nbins_rec, 0, 4, nbins_gen, 0, 4) # rec = x, gen = y
    gen = ROOT.TH1D('gen', 'gen', nbins_gen, 0, 4)
    bkg = ROOT.TH1D('bkg', 'bkg', nbins_rec, 0, 4)
    data = ROOT.TH1D('data', 'data', nbins_rec, 0, 4)

    for igen in range(nbins_gen):
        gen.SetBinContent(igen + 1, gen_tmp.GetBinContent(igen + 1))

    # overflow bin index: This bin contains the events reconstructed with no gen info, so that's the 'internal' background.
    gen_ioverflow = response_tmp.GetYaxis().GetNbins() + 1
    for irec in range(nbins_rec):
        bkg.SetBinContent(irec + 1, response_tmp.GetBinContent(irec + 1, gen_ioverflow))

    for irec in range(nbins_rec):
        for igen in range(nbins_gen):
            response.SetBinContent(irec+1, igen+1, response_tmp.GetBinContent(irec+1, igen+1) / gen.GetBinContent(igen+1))
       
    # fill background and data histograms. the background template is given by projecting the response histo for the background processes onto the x-axis (=reco),
    # including the overflow bin:
    bkg_tmp = response_bkg_tmp.ProjectionX("_px", 0, gen_ioverflow)
    bkg_tmp.SetDirectory(0)
    for irec in range(nbins_rec):
        bkg.SetBinContent(irec + 1, bkg.GetBinContent(irec + 1) + bkg_tmp.GetBinContent(irec + 1))
        data.SetBinContent(irec + 1, response_data_tmp.GetBinContent(irec + 1, gen_ioverflow))
    
    gen.SetDirectory(outdir)
    response.SetDirectory(outdir)
    bkg.SetDirectory(outdir)
    data.SetDirectory(outdir)
    outdir.Write()

outfile = ROOT.TFile('../unfold/zsv-mm.root', 'recreate')

#create_hists('mm', 'dy', outfile)
#create_hists('ee', 'dy', outfile)

create_hists('mm_met', 'dy', outfile)
#create_hists('ee_met', 'dy', outfile)

outfile.Write()
outfile.Close()

print 'done'

