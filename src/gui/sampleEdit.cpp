/**
 * Furnace Tracker - multi-system chiptune tracker
 * Copyright (C) 2021-2022 tildearrow and contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "gui.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <math.h>
#include "../ta-log.h"
#include "IconsFontAwesome4.h"
#include "misc/cpp/imgui_stdlib.h"
#include <fmt/printf.h>
#include "guiConst.h"

void FurnaceGUI::drawSampleEdit() {
  if (nextWindow==GUI_WINDOW_SAMPLE_EDIT) {
    sampleEditOpen=true;
    ImGui::SetNextWindowFocus();
    nextWindow=GUI_WINDOW_NOTHING;
  }
  if (!sampleEditOpen) return;
  if (ImGui::Begin("Sample Editor",&sampleEditOpen,settings.allowEditDocking?0:ImGuiWindowFlags_NoDocking)) {
    if (curSample<0 || curSample>=(int)e->song.sample.size()) {
      ImGui::Text("no sample selected");
    } else {
      DivSample* sample=e->song.sample[curSample];
      String sampleType="Invalid";
      if (sample->depth<17) {
        if (sampleDepths[sample->depth]!=NULL) {
          sampleType=sampleDepths[sample->depth];
        }
      }
      ImGui::Text("Name");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
      ImGui::InputText("##SampleName",&sample->name);

      if (ImGui::BeginTable("SampleProps",4,ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Type");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::BeginCombo("##SampleType",sampleType.c_str())) {
          for (int i=0; i<17; i++) {
            if (sampleDepths[i]==NULL) continue;
            if (ImGui::Selectable(sampleDepths[i])) {
              sample->depth=i;
              e->renderSamplesP();
              updateSampleTex=true;
            }
            if (ImGui::IsItemHovered()) {
              ImGui::SetTooltip("no undo for sample type change operations!");
            }
          }
          ImGui::EndCombo();
        }

        ImGui::TableNextColumn();
        ImGui::Text("Rate (Hz)");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::InputInt("##SampleRate",&sample->rate,10,200)) {
          if (sample->rate<100) sample->rate=100;
          if (sample->rate>96000) sample->rate=96000;
        }

        ImGui::TableNextColumn();
        ImGui::Text("C-4 (Hz)");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::InputInt("##SampleCenter",&sample->centerRate,10,200)) {
          if (sample->centerRate<100) sample->centerRate=100;
          if (sample->centerRate>65535) sample->centerRate=65535;
        }

        ImGui::TableNextColumn();
        bool doLoop=(sample->loopStart>=0);
        if (ImGui::Checkbox("Loop",&doLoop)) {
          if (doLoop) {
            sample->loopStart=0;
          } else {
            sample->loopStart=-1;
          }
          updateSampleTex=true;
        }
        if (doLoop) {
          ImGui::SameLine();
          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
          if (ImGui::InputInt("##LoopPosition",&sample->loopStart,1,10)) {
            if (sample->loopStart<0 || sample->loopStart>=(int)sample->samples) {
              sample->loopStart=0;
            }
            updateSampleTex=true;
          }
        }
        ImGui::EndTable();
      }

      /*
      if (ImGui::Button("Apply")) {
        e->renderSamplesP();
      }
      ImGui::SameLine();
      */
      ImGui::Separator();

      if (ImGui::Button(ICON_FA_I_CURSOR "##SSelect")) {
        sampleDragMode=false;
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Edit mode: Select");
      }
      ImGui::SameLine();
      if (ImGui::Button(ICON_FA_PENCIL "##SDraw")) {
        sampleDragMode=true;
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Edit mode: Draw");
      }
      ImGui::SameLine();
      ImGui::Dummy(ImVec2(4.0*dpiScale,dpiScale));
      ImGui::SameLine();
      ImGui::Button(ICON_FA_ARROWS_H "##SResize");
      if (ImGui::IsItemClicked()) {
        resizeSize=sample->samples;
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Resize");
      }
      if (ImGui::BeginPopupContextItem("SResizeOpt",ImGuiPopupFlags_MouseButtonLeft)) {
        if (ImGui::InputInt("Samples",&resizeSize,1,64)) {
          if (resizeSize<0) resizeSize=0;
          if (resizeSize>16777215) resizeSize=16777215;
        }
        if (ImGui::Button("Resize")) {
          e->synchronized([this,sample]() {
            if (!sample->resize(resizeSize)) {
              showError("couldn't resize! make sure your sample is 8 or 16-bit.");
            }
            e->renderSamples();
          });
          updateSampleTex=true;
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      } else {
        resizeSize=sample->samples;
      }
      ImGui::SameLine();
      ImGui::Button(ICON_FA_EXPAND "##SResample");
      if (ImGui::IsItemClicked()) {
        resampleTarget=sample->rate;
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Resample");
      }
      if (ImGui::BeginPopupContextItem("SResampleOpt",ImGuiPopupFlags_MouseButtonLeft)) {
        ImGui::Text("Rate");
        if (ImGui::InputDouble("##SRRate",&resampleTarget,1.0,50.0,"%g")) {
          if (resampleTarget<0) resampleTarget=0;
          if (resampleTarget>96000) resampleTarget=96000;
        }
        ImGui::SameLine();
        if (ImGui::Button("0.5x")) {
          resampleTarget*=0.5;
        }
        ImGui::SameLine();
        if (ImGui::Button("==")) {
          resampleTarget=sample->rate;
        }
        ImGui::SameLine();
        if (ImGui::Button("2.0x")) {
          resampleTarget*=2.0;
        }
        double factor=resampleTarget/(double)sample->rate;
        if (ImGui::InputDouble("Factor",&factor,0.125,0.5,"%g")) {
          resampleTarget=(double)sample->rate*factor;
          if (resampleTarget<0) resampleTarget=0;
          if (resampleTarget>96000) resampleTarget=96000;
        }
        ImGui::Combo("Filter",&resampleStrat,resampleStrats,6);
        if (ImGui::Button("Resample")) {
          e->synchronized([this,sample]() {
            if (!sample->resample(resampleTarget,resampleStrat)) {
              showError("couldn't resample! make sure your sample is 8 or 16-bit.");
            }
            e->renderSamples();
          });
          updateSampleTex=true;
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      } else {
        resampleTarget=sample->rate;
      }
      ImGui::SameLine();
      ImGui::Dummy(ImVec2(4.0*dpiScale,dpiScale));
      ImGui::SameLine();
      ImGui::Button(ICON_FA_VOLUME_UP "##SAmplify");
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Amplify");
      }
      if (ImGui::BeginPopupContextItem("SAmplifyOpt",ImGuiPopupFlags_MouseButtonLeft)) {
        ImGui::Text("Volume");
        if (ImGui::InputFloat("##SRVolume",&amplifyVol,10.0,50.0,"%g%%")) {
          if (amplifyVol<0) amplifyVol=0;
          if (amplifyVol>10000) amplifyVol=10000;
        }
        ImGui::SameLine();
        ImGui::Text("(%.1fdB)",20.0*log10(amplifyVol/100.0f));
        ImGui::Button("Apply");
        ImGui::EndPopup();
      }
      ImGui::SameLine();
      ImGui::Button(ICON_FA_ARROWS_V "##SNormalize");
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Normalize");
      }
      ImGui::SameLine();
      ImGui::Button(ICON_FA_ERASER "##SSilence");
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Apply silence");
      }
      ImGui::SameLine();
      ImGui::Dummy(ImVec2(4.0*dpiScale,dpiScale));
      ImGui::SameLine();
      ImGui::Button(ICON_FA_BACKWARD "##SReverse");
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Reverse");
      }
      ImGui::SameLine();
      ImGui::Button(ICON_FA_SORT_AMOUNT_ASC "##SInvert");
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Invert");
      }
      ImGui::SameLine();
      ImGui::Button(ICON_FA_LEVEL_DOWN "##SSign");
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Signed/unsigned exchange");
      }
      ImGui::SameLine();
      ImGui::Button(ICON_FA_INDUSTRY "##SFilter");
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Apply filter");
      }

      ImGui::SameLine();
      ImGui::Dummy(ImVec2(4.0*dpiScale,dpiScale));
      ImGui::SameLine();
      if (ImGui::Button(ICON_FA_VOLUME_UP "##PreviewSample")) {
        e->previewSample(curSample);
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Preview sample");
      }
      ImGui::SameLine();
      if (ImGui::Button(ICON_FA_VOLUME_OFF "##StopSample")) {
        e->stopSamplePreview();
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Stop sample preview");
      }

      ImGui::SameLine();
      double zoomPercent=100.0/sampleZoom;
      ImGui::Text("Zoom");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(150.0f*dpiScale);
      if (ImGui::InputDouble("##SZoom",&zoomPercent,5.0,20.0,"%g%%")) {
        if (zoomPercent>10000.0) zoomPercent=10000.0;
        if (zoomPercent<1.0) zoomPercent=1.0;
        sampleZoom=100.0/zoomPercent;
        if (sampleZoom<0.01) sampleZoom=0.01;
        sampleZoomAuto=false;
        updateSampleTex=true;
      }
      ImGui::SameLine();
      if (sampleZoomAuto) {
        if (ImGui::Button("100%")) {
          sampleZoom=1.0;
          sampleZoomAuto=false;
          updateSampleTex=true;
        }
      } else {
        if (ImGui::Button("Auto")) {
          sampleZoomAuto=true;
          updateSampleTex=true;
        }
      }

      ImGui::Separator();

      ImVec2 avail=ImGui::GetContentRegionAvail();
      avail.y-=ImGui::GetFontSize()+ImGui::GetStyle().ItemSpacing.y+ImGui::GetStyle().ScrollbarSize;
      int availX=avail.x;
      int availY=avail.y;


      if (sampleZoomAuto) {
        samplePos=0;
        if (sample->samples<1 || avail.x<=0) {
          sampleZoom=1.0;
        } else {
          sampleZoom=(double)sample->samples/avail.x;
        }
        if (sampleZoom!=prevSampleZoom) {
          prevSampleZoom=sampleZoom;
          updateSampleTex=true;
        }
      }

      if (sampleTex==NULL || sampleTexW!=avail.x || sampleTexH!=avail.y) {
        if (sampleTex!=NULL) {
          SDL_DestroyTexture(sampleTex);
          sampleTex=NULL;
        }
        if (avail.x>=1 && avail.y>=1) {
          logD("recreating sample texture.\n");
          sampleTex=SDL_CreateTexture(sdlRend,SDL_PIXELFORMAT_ABGR8888,SDL_TEXTUREACCESS_STREAMING,avail.x,avail.y);
          sampleTexW=avail.x;
          sampleTexH=avail.y;
          if (sampleTex==NULL) {
            logE("error while creating sample texture! %s\n",SDL_GetError());
          } else {
            updateSampleTex=true;
          }
        }
      }

      if (sampleTex!=NULL) {
        if (updateSampleTex) {
          unsigned int* data=NULL;
          int pitch=0;
          logD("updating sample texture.\n");
          if (SDL_LockTexture(sampleTex,NULL,(void**)&data,&pitch)!=0) {
            logE("error while locking sample texture! %s\n",SDL_GetError());
          } else {
            ImU32 bgColor=ImGui::GetColorU32(ImGuiCol_FrameBg);
            ImU32 lineColor=ImGui::GetColorU32(ImGuiCol_PlotLines);
            for (int i=0; i<availX*availY; i++) {
              data[i]=bgColor;
            }
            unsigned int xCoarse=samplePos;
            unsigned int xFine=0;
            unsigned int xAdvanceCoarse=sampleZoom;
            unsigned int xAdvanceFine=fmod(sampleZoom,1.0)*16777216;
            for (unsigned int i=0; i<(unsigned int)availX; i++) {
              if (xCoarse>=sample->samples) break;
              int y1, y2;
              int totalAdvance=0;
              if (sample->depth==8) {
                y1=((unsigned char)sample->data8[xCoarse]^0x80)*availY/256;
              } else {
                y1=((unsigned short)sample->data16[xCoarse]^0x8000)*availY/65536;
              }
              xFine+=xAdvanceFine;
              if (xFine>=16777216) {
                xFine-=16777216;
                totalAdvance++;
              }
              totalAdvance+=xAdvanceCoarse;
              if (xCoarse>=sample->samples) break;
              do {
                if (sample->depth==8) {
                  y2=((unsigned char)sample->data8[xCoarse]^0x80)*availY/256;
                } else {
                  y2=((unsigned short)sample->data16[xCoarse]^0x8000)*availY/65536;
                }
                if (y1>y2) {
                  y2^=y1;
                  y1^=y2;
                  y2^=y1;
                }
                if (y1<0) y1=0;
                if (y1>=availY) y1=availY-1;
                if (y2<0) y2=0;
                if (y2>=availY) y2=availY-1;
                for (int j=y1; j<=y2; j++) {
                  data[i+availX*(availY-j-1)]=lineColor;
                }
                if (totalAdvance>0) xCoarse++;
              } while ((totalAdvance--)>0);
            }
            SDL_UnlockTexture(sampleTex);
          }
          updateSampleTex=false;
        }

        ImGui::ImageButton(sampleTex,avail,ImVec2(0,0),ImVec2(1,1),0);

        ImVec2 rectMin=ImGui::GetItemRectMin();
        ImVec2 rectMax=ImGui::GetItemRectMax();
        ImVec2 rectSize=ImGui::GetItemRectSize();

        if (ImGui::IsItemClicked()) {
          if (sample->samples>0 && (sample->depth==16 || sample->depth==8)) {
            sampleDragStart=rectMin;
            sampleDragAreaSize=rectSize;
            sampleDrag16=(sample->depth==16);
            sampleDragTarget=(sample->depth==16)?((void*)sample->data16):((void*)sample->data8);
            sampleDragLen=sample->samples;
            sampleDragActive=true;
            sampleSelStart=-1;
            sampleSelEnd=-1;
            processDrags(ImGui::GetMousePos().x,ImGui::GetMousePos().y);
          }
        }
        String statusBar=sampleDragMode?"Draw":"Select";
        bool drawSelection=false;

        if (!sampleDragMode) {
          if (sampleSelStart>=0 && sampleSelEnd>=0 && sampleSelStart!=sampleSelEnd) {
            int start=sampleSelStart;
            int end=sampleSelEnd;
            if (start>end) {
              start^=end;
              end^=start;
              start^=end;
            }
            statusBar+=fmt::sprintf(" (%d-%d)",start,end);
            drawSelection=true;
          }
        }

        if (ImGui::IsItemHovered()) {
          int posX=-1;
          int posY=0;
          ImVec2 pos=ImGui::GetMousePos();
          pos.x-=rectMin.x;
          pos.y-=rectMin.y;

          if (sampleZoom>0) {
            posX=samplePos+pos.x*sampleZoom;
            if (posX>(int)sample->samples) posX=-1;
          }
          posY=(0.5-pos.y/rectSize.y)*((sample->depth==8)?255:32767);
          if (posX>=0) {
            statusBar+=fmt::sprintf(" | (%d, %d)",posX,posY);
          }
        }

        if (drawSelection) {
          int start=sampleSelStart;
          int end=sampleSelEnd;
          if (start>end) {
            start^=end;
            end^=start;
            start^=end;
          }
          ImDrawList* dl=ImGui::GetWindowDrawList();
          ImVec2 p1=rectMin;
          p1.x+=start/sampleZoom-samplePos;

          ImVec2 p2=ImVec2(rectMin.x+end/sampleZoom-samplePos,rectMax.y);
          ImVec4 selColor=uiColors[GUI_COLOR_ACCENT_SECONDARY];
          selColor.w*=0.25;

          dl->AddRectFilled(p1,p2,ImGui::GetColorU32(selColor));
        }

        ImS64 scrollV=samplePos;
        ImS64 availV=round(rectSize.x*sampleZoom);
        ImS64 contentsV=MAX(sample->samples,MAX(availV,1));

        if (ImGui::ScrollbarEx(ImRect(ImVec2(rectMin.x,rectMax.y),ImVec2(rectMax.x,rectMax.y+ImGui::GetStyle().ScrollbarSize)),ImGui::GetID("sampleScroll"),ImGuiAxis_X,&scrollV,availV,contentsV,0)) {
          if (!sampleZoomAuto && samplePos!=scrollV) {
            samplePos=scrollV;
            updateSampleTex=true;
          }
        }
        
        ImGui::SetCursorPosY(ImGui::GetCursorPosY()+ImGui::GetStyle().ScrollbarSize);
        ImGui::Text("%s",statusBar.c_str());
      }

      /*
      bool considerations=false;
      ImGui::Text("notes:");
      if (sample->loopStart>=0) {
        considerations=true;
        ImGui::Text("- sample won't loop on Neo Geo ADPCM-A and X1-010");
        if (sample->loopStart&1) {
          ImGui::Text("- sample loop start will be aligned to the nearest even sample on Amiga");
        }
        if (sample->loopStart>0) {
          ImGui::Text("- sample loop start will be ignored on Neo Geo ADPCM-B");
        }
      }
      if (sample->samples&1) {
        considerations=true;
        ImGui::Text("- sample length will be aligned to the nearest even sample on Amiga");
      }
      if (sample->samples&511) {
        considerations=true;
        ImGui::Text("- sample length will be aligned and padded to 512 sample units on Neo Geo ADPCM.");
      }
      if (sample->samples&4095) {
        considerations=true;
        ImGui::Text("- sample length will be aligned and padded to 4096 sample units on X1-010.");
      }
      if (sample->samples>65535) {
        considerations=true;
        ImGui::Text("- maximum sample length on Sega PCM and QSound is 65536 samples");
      }
      if (sample->samples>131071) {
        considerations=true;
        ImGui::Text("- maximum sample length on X1-010 is 131072 samples");
      }
      if (sample->samples>2097151) {
        considerations=true;
        ImGui::Text("- maximum sample length on Neo Geo ADPCM is 2097152 samples");
      }
      if (!considerations) {
        ImGui::Text("- none");
      }*/
    }
  }
  if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) curWindow=GUI_WINDOW_SAMPLE_EDIT;
  ImGui::End();
}
