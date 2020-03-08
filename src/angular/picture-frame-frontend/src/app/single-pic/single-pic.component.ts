import { Component, OnInit, ChangeDetectionStrategy } from '@angular/core';

@Component({
  selector: 'app-single-pic',
  templateUrl: './single-pic.component.html',
  styleUrls: ['./single-pic.component.scss'],
  changeDetection: ChangeDetectionStrategy.OnPush
})
export class SinglePicComponent implements OnInit {

  constructor() { }

  ngOnInit(): void {
  }

}
