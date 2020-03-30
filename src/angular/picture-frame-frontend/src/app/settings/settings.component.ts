import { Component, OnInit, ChangeDetectionStrategy, ChangeDetectorRef } from '@angular/core';
import * as asciichart from 'asciichart';
import { timer } from 'rxjs';
import { SystemService } from '../core/system.service';
import { environment } from 'src/environments/environment';


@Component({
  selector: 'app-settings',
  templateUrl: './settings.component.html',
  styleUrls: ['./settings.component.scss'],
  changeDetection: ChangeDetectionStrategy.OnPush
})
export class SettingsComponent implements OnInit {
  plotSize = 60;
  heapFifo: number[] = [];
  isWaiting = false;
  plot = 'initial';
  plotArray: string[];

  constructor(
    private changeRef: ChangeDetectorRef,
    private systemService: SystemService,
  ) { }

  ngOnInit(): void {
    if (!environment.production) {
      console.log('not production');
      this.mockPlot();
    } else {
      console.log('production');
      for (let i = 1; i <= this.plotSize; i++) {
        this.heapFifo.push(0);
      }
      const source = timer(2000, 2000);
      const timeSeries = source.subscribe(
        (timerValue) => {
          this.updateGraph();
        });
    }
  }

  private mockPlot() {
    for (let i = 1; i <= this.plotSize; i++) {
      this.heapFifo.push(Math.random() * 100);
    }
    this.plot = asciichart.plot(
      this.heapFifo,
      {
        height: 10,
      });
    this.plotArray = this.plot.split('\n');
    this.changeRef.detectChanges();
  }

  private updateGraph() {
    // Only poll once at a time
    if (!this.isWaiting) {
      this.isWaiting = true;
      this.systemService.getHeap().subscribe(
        (heap: string) => {
          this.heapFifo.shift();
          this.heapFifo.push(Number(heap));
          this.isWaiting = false;
        }
      );
      this.plot = asciichart.plot(
        this.heapFifo,
        {
          height: 10,
        });
      this.plotArray = this.plot.split('\n');
      this.changeRef.detectChanges();
    }
  }

}
