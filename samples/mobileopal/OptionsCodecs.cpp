/*
 * OptionsVideo.cpp
 *
 * Sample Windows Mobile application.
 *
 * Open Phone Abstraction Library
 *
 * Copyright (c) 2009 Vox Lucida
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Vox Lucida (Robert Jongbloed)
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include "stdafx.h"
#include "MobileOPAL.h"
#include "OptionsCodecs.h"


// COptionsCodecs dialog

IMPLEMENT_DYNAMIC(COptionsCodecs, CScrollableDialog)

COptionsCodecs::COptionsCodecs(CWnd* pParent /*=NULL*/)
  : CScrollableDialog(COptionsCodecs::IDD, pParent)
  , m_AutoStartTxVideo(FALSE)
  , m_hEnabledCodecs(NULL)
  , m_hDisabledCodecs(NULL)
  , m_hDraggedItem(NULL)
  , m_bDraggingNow(false)
  , m_pDragImageList(NULL)
{

}

COptionsCodecs::~COptionsCodecs()
{
}

void COptionsCodecs::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Check(pDX, IDC_AUTO_START_TX_VIDEO, m_AutoStartTxVideo);
  DDX_Control(pDX, IDC_CODEC_LIST, m_CodecInfo);
}


BEGIN_MESSAGE_MAP(COptionsCodecs, CDialog)
  ON_NOTIFY(TVN_BEGINLABELEDIT, IDC_CODEC_LIST, &COptionsCodecs::OnTvnBeginlabeleditCodecList)
  ON_NOTIFY(TVN_ENDLABELEDIT, IDC_CODEC_LIST, &COptionsCodecs::OnTvnEndlabeleditCodecList)
  ON_NOTIFY(TVN_BEGINDRAG, IDC_CODEC_LIST, &COptionsCodecs::OnTvnBegindragCodecList)
  ON_WM_MOUSEMOVE()
  ON_WM_LBUTTONUP()
END_MESSAGE_MAP()


static void SplitString(const CString & str, CMediaInfoList & list)
{
  int tokenPos = 0;
  CString token;
  while (!(token = str.Tokenize(L"\n", tokenPos)).IsEmpty()) {
    CMediaInfo info;

    int colon = token.Find(':');
    if (colon < 0)
      info = token;
    else {
      info = token.Left(colon);

      int equal = token.FindOneOf(L"#=");
      if (equal < 0)
        info.m_options[token.Mid(colon+1)] = CMediaInfo::OptionInfo();
      else
        info.m_options[token.Mid(colon+1, equal-colon-1)] = CMediaInfo::OptionInfo(token.Mid(equal+1), token[equal] == '#');
    }

    CMediaInfoList::iterator iter = std::find(list.begin(), list.end(), info);
    if (iter == list.end())
      list.push_back(info);
    else if (!info.m_options.empty())
      iter->m_options[info.m_options.begin()->first] = info.m_options.begin()->second;
  }
}


BOOL COptionsCodecs::OnInitDialog()
{
  CScrollableDialog::OnInitDialog();

  m_hEnabledCodecs = m_CodecInfo.InsertItem(L"Enabled Codecs");
  m_hDisabledCodecs = m_CodecInfo.InsertItem(L"Disabled Codecs");

  CMediaInfoList order;
  SplitString(m_MediaOrder, order);

  CMediaInfoList mask;
  SplitString(m_MediaMask, mask);

  CMediaInfoList options;
  SplitString(m_MediaOptions, options);

  CMediaInfoList all;
  SplitString(m_AllMediaOptions, all);

  CMediaInfoList::const_iterator iter;
  for (iter = order.begin(); iter != order.end(); ++iter) {
    CMediaInfoList::iterator full = std::find(all.begin(), all.end(), *iter);
    if (full != all.end())
      AddCodec(*full, mask, options);
  }

  for (iter = all.begin(); iter != all.end(); ++iter) {
    if (std::find(order.begin(), order.end(), *iter) == order.end())
      AddCodec(*iter, mask, options);
  }

  m_CodecInfo.Expand(m_hEnabledCodecs, TVE_EXPAND);

  return TRUE;
}


void COptionsCodecs::AddCodec(const CMediaInfo & info, const CMediaInfoList & mask, const CMediaInfoList & options)
{
  CMediaInfo adjustedInfo = info;

  CMediaInfoList::const_iterator overrides = std::find(options.begin(), options.end(), info);
  if (overrides != options.end()) {
    for (CMediaInfo::OptionMap::const_iterator iter = overrides->m_options.begin(); iter != overrides->m_options.end(); ++iter)
      adjustedInfo.m_options[iter->first] = iter->second;
  }

  HTREEITEM hFormat = m_CodecInfo.InsertItem(adjustedInfo,
                    std::find(mask.begin(), mask.end(), info) != mask.end() ? m_hDisabledCodecs : m_hEnabledCodecs);
  m_CodecInfo.SetItemData(hFormat, 2);

  for (CMediaInfo::OptionMap::const_iterator iter = adjustedInfo.m_options.begin(); iter != adjustedInfo.m_options.end(); ++iter) {
    HTREEITEM hOptVal = m_CodecInfo.InsertItem(iter->second.m_value, m_CodecInfo.InsertItem(iter->first, hFormat));
    if (iter->second.m_readOnly)
      m_CodecInfo.SetItemState(hOptVal, TVIS_BOLD, TVIS_BOLD);
    else
      m_CodecInfo.SetItemData(hOptVal, 1);
  }
}


// COptionsCodecs message handlers

void COptionsCodecs::OnTvnBeginlabeleditCodecList(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
  *pResult = pTVDispInfo->item.cChildren != 0 || pTVDispInfo->item.lParam != 1;
}


void COptionsCodecs::OnTvnEndlabeleditCodecList(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);

  if (pTVDispInfo->item.pszText == NULL)
    return;

  HTREEITEM hOption = m_CodecInfo.GetParentItem(pTVDispInfo->item.hItem);
  HTREEITEM hFormat = m_CodecInfo.GetParentItem(hOption);
  CString prefix = m_CodecInfo.GetItemText(hFormat) + ':' + m_CodecInfo.GetItemText(hOption) + '=';

  int pos = m_MediaOptions.Find(prefix);
  if (pos < 0) {
    if (!m_MediaOptions.IsEmpty())
      m_MediaOptions += '\n';
    m_MediaOptions += prefix + pTVDispInfo->item.pszText;
  }
  else {
    pos += prefix.GetLength();
    int end = m_MediaOptions.Find('\n', pos);
    if (end < 0)
      end = m_MediaOptions.GetLength();
    m_MediaOptions.Delete(pos, end-pos);
    m_MediaOptions.Insert(pos, pTVDispInfo->item.pszText);
  }

  *pResult = 1;
}


void COptionsCodecs::OnTvnBegindragCodecList(NMHDR *pNMHDR, LRESULT *pResult)
{
  *pResult = 0;
  LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

  if (m_bDraggingNow)
    return;

  if (pNMTreeView->itemNew.lParam != 2)
    return;

}

void COptionsCodecs::OnMouseMove(UINT nFlags, CPoint point)
{
}

void COptionsCodecs::OnLButtonUp(UINT nFlags, CPoint point)
{
}
